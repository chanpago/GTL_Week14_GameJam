#include "pch.h"
#include "AnimBlueprintCompiler.h"
#include "AnimationGraph.h"
#include "BlueprintEvaluator.h"
#include "EdGraphNode.h"
#include "K2Node_Animation.h"
#include "Source/Runtime/Engine/Animation/AnimInstance.h"
#include "Source/Runtime/Engine/Animation/AnimationStateMachine.h"
#include "Source/Editor/FBX/BlendSpace/BlendSpace1D.h"
#include "Source/Editor/FBX/BlendSpace/BlendSpace2D.h"

void FAnimBlueprintCompiler::Compile(UAnimationGraph* InGraph, UAnimInstance* InAnimInstance, UAnimationStateMachine* OutStateMachine)
{
    if (!InGraph)
    {
        return;
    }

    OutStateMachine->Clear();

    TMap<UEdGraphNode*, FName> NodeToStateMap;
    
    FBlueprintContext Context(InAnimInstance);

    // --- #1. 모든 상태 노드 등록 ---
    for (UEdGraphNode* Node : InGraph->Nodes)
    {
        if (UK2Node_AnimState* StateNode = Cast<UK2Node_AnimState>(Node))
        {
            bool bLoop = FBlueprintEvaluator::EvaluateInput<bool>(StateNode->FindPin("Looping"), &Context);
            float PlayRate = FBlueprintEvaluator::EvaluateInput<float>(StateNode->FindPin("PlayRate"), &Context);
            FName StateName = FName(StateNode->StateName);

            // Animation 핀에서 값을 가져옴 (AnimSequence 또는 BlendSpace1D)
            UEdGraphPin* AnimPin = StateNode->FindPin("Animation");
            FAnimationState NewState;
            NewState.Name = StateName;
            NewState.bLoop = bLoop;
            NewState.PlayRate = PlayRate;
            NewState.bEnableRootMotion = StateNode->bEnableRootMotion;

            if (AnimPin && AnimPin->LinkedTo.Num() > 0)
            {
                UEdGraphPin* SourcePin = AnimPin->LinkedTo[0];
                if (SourcePin && SourcePin->OwningNode)
                {
                    FBlueprintValue Result = SourcePin->OwningNode->EvaluatePin(SourcePin, &Context);

                    // AnimSequence인지 BlendSpace1D/2D인지 확인
                    if (std::holds_alternative<UAnimSequence*>(Result.Value))
                    {
                        UAnimSequence* AnimSeq = Result.Get<UAnimSequence*>();
                        NewState.Sequence = AnimSeq;
                        NewState.PoseProvider = AnimSeq;
                    }
                    else if (std::holds_alternative<UBlendSpace1D*>(Result.Value))
                    {
                        UBlendSpace1D* BlendSpace = Result.Get<UBlendSpace1D*>();
                        NewState.Sequence = nullptr;
                        NewState.PoseProvider = BlendSpace;
                    }
                    else if (std::holds_alternative<UBlendSpace2D*>(Result.Value))
                    {
                        UBlendSpace2D* BlendSpace = Result.Get<UBlendSpace2D*>();
                        NewState.Sequence = nullptr;
                        NewState.PoseProvider = BlendSpace;
                    }

                    UEdGraphNode* SourceNode = SourcePin->OwningNode;
                    
                    NewState.OnUpdate = [SourceNode, SourcePin, InAnimInstance]()
                    {
                        if (!SourceNode || !InAnimInstance) return;

                        // 런타임 컨텍스트 생성
                        FBlueprintContext RuntimeContext(InAnimInstance);

                        // 노드 재평가 -> 내부적으로 입력 핀을 읽고 SetParameter 등을 수행함
                        // (반환값은 무시하고, 객체의 내부 상태 갱신이 목적)
                        SourceNode->EvaluatePin(SourcePin, &RuntimeContext);
                    };
                }
            }
            else
            {
                // 연결이 없으면 기존 방식으로 시도
                UAnimSequence* AnimSeq = FBlueprintEvaluator::EvaluateInput<UAnimSequence*>(AnimPin, &Context);
                NewState.Sequence = AnimSeq;
                NewState.PoseProvider = AnimSeq;
            }

            OutStateMachine->AddState(NewState);
            NodeToStateMap.Add(StateNode, StateName);
        }
    }

    // --- #2. 진입점 생성 ---
    UK2Node_AnimStateEntry* EntryNode = InGraph->GetEntryNode();
    if (EntryNode)
    {
        if (UEdGraphPin* ExecPin = EntryNode->FindPin("Entry", EEdGraphPinDirection::EGPD_Output))
        {
            UEdGraphNode* NextNode = GetConnectedNode(ExecPin);
            if (NextNode && NodeToStateMap.Contains(NextNode))
            {
                OutStateMachine->SetInitialState(NodeToStateMap[NextNode]);
            }
        }
    }

    // --- #3. 전이 규칙 등록 ---
    for (UEdGraphNode* Node : InGraph->Nodes)
    {
        if (UK2Node_AnimTransition* TransitionNode = Cast<UK2Node_AnimTransition>(Node))
        {
            UEdGraphPin* InputExec = TransitionNode->FindPin("Execute", EEdGraphPinDirection::EGPD_Input);
            UEdGraphPin* OutputExec = TransitionNode->FindPin("Transition To", EEdGraphPinDirection::EGPD_Output);

            UEdGraphNode* FromNode = GetConnectedNode(InputExec);
            UEdGraphNode* ToNode = GetConnectedNode(OutputExec);

            if (FromNode && ToNode && NodeToStateMap.Contains(FromNode) && NodeToStateMap.Contains(ToNode))
            {
                FName FromName = NodeToStateMap[FromNode];
                FName ToName = NodeToStateMap[ToNode];

                float BlendTime = FBlueprintEvaluator::EvaluateInput<float>(TransitionNode->FindPin("Blend Time"), &Context);

                auto Condition = [TransitionNode, InAnimInstance]() -> bool
                {
                    if (!TransitionNode || !InAnimInstance)
                    {
                        return false;
                    }

                    FBlueprintContext RuntimeContext(InAnimInstance);

                    return FBlueprintEvaluator::EvaluateInput<bool>(
                        TransitionNode->FindPin("Can Transition"),
                        &RuntimeContext
                    );
                };

                OutStateMachine->AddTransition(FStateTransition(FromName, ToName, Condition, BlendTime));
            }
        }
    }
}

UEdGraphNode* FAnimBlueprintCompiler::GetConnectedNode(UEdGraphPin* Pin)
{
    if (!Pin || Pin->LinkedTo.Num() == 0)
    {
        return nullptr;
    }
    return Pin->LinkedTo[0]->OwningNode;
}
