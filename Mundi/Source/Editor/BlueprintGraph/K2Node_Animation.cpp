#include "pch.h"
#include "ImGui/imgui_stdlib.h"
#include "imgui-node-editor/imgui_node_editor.h"
#include "K2Node_Animation.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintEvaluator.h"
#include "Source/Editor/FBX/BlendSpace/BlendSpace1D.h"

namespace ed = ax::NodeEditor;

// ----------------------------------------------------------------
//	[AnimSequence] 애니메이션 시퀀스 노드 
// ----------------------------------------------------------------

IMPLEMENT_CLASS(UK2Node_AnimSequence, UK2Node)

UK2Node_AnimSequence::UK2Node_AnimSequence()
{
    TitleColor = ImColor(100, 120, 255);
}

void UK2Node_AnimSequence::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    UK2Node::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        FString AnimPath;
        if (FJsonSerializer::ReadString(InOutHandle, "AnimPath", AnimPath))
        {
            if (!AnimPath.empty())
            {
                Value = RESOURCE.Get<UAnimSequence>(AnimPath);
            }
        }
    }
    else
    {
        if (Value)
        {
            InOutHandle["AnimPath"] = Value->GetFilePath();
        }
    }
}

void UK2Node_AnimSequence::AllocateDefaultPins()
{
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::AnimSequence, "Value");
}

/**
 * @note ed::Suspend를 활용하지 않으면 스크린 공간이 아닌 캔버스 공간 좌표가 사용되어서
 * 정상적으로 창이 뜨지 않는 이슈 존재가 존재했었다. 현재는 해결되었다.
 */
void UK2Node_AnimSequence::RenderBody()
{
    FString PopupID = "##AnimSelectPopup_";
    PopupID += std::to_string(NodeID);

    FString PreviewName = "None";
    if (Value)
    {
        PreviewName = Value->GetFilePath();
    }

    if (ImGui::Button(PreviewName.c_str()))
    {
        ed::Suspend();
        ImGui::OpenPopup(PopupID.c_str());
        ed::Resume();
    }

    if (ImGui::IsPopupOpen(PopupID.c_str()))
    {
        ed::Suspend();
        
        if (ImGui::BeginPopup(PopupID.c_str()))
        {
            TArray<UAnimSequence*> AnimSequences = RESOURCE.GetAll<UAnimSequence>();

            for (UAnimSequence* Anim : AnimSequences)
            {
                if (!Anim) continue;

                const FString AssetName = Anim->GetFilePath();
                bool bIsSelected = (Value == Anim);
                  
                if (ImGui::Selectable(AssetName.c_str(), bIsSelected))
                {
                    Value = Anim;
                    // LoadMeta 호출 제거 - GetAnimNotifyEvents()가 이미 lazy loading을 수행함
                    // 불필요한 LoadMeta 호출은 메모리의 노티파이를 파일 데이터로 덮어씀
                    ImGui::CloseCurrentPopup();
                }

                if (bIsSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            
            ImGui::EndPopup();
        }
        ed::Resume();
    }
}

FBlueprintValue UK2Node_AnimSequence::EvaluatePin(const UEdGraphPin* OutputPin, FBlueprintContext* Context)
{
    if (OutputPin->PinName == "Value")
    {
        return Value;
    }
    return static_cast<UAnimSequence*>(nullptr);
}

// ----------------------------------------------------------------
//	[AnimStateEntry] 애니메이션 상태 머신 진입점
// ----------------------------------------------------------------

IMPLEMENT_CLASS(UK2Node_AnimStateEntry, UK2Node)

UK2Node_AnimStateEntry::UK2Node_AnimStateEntry()
{
    TitleColor = ImColor(150, 150, 150);
}

void UK2Node_AnimStateEntry::AllocateDefaultPins()
{
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Exec, "Entry"); 
}

void UK2Node_AnimStateEntry::RenderBody()
{
}

void UK2Node_AnimStateEntry::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());

    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();

    ActionRegistrar.AddAction(Spawner);
}

void UK2Node_AnimSequence::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());

    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();

    ActionRegistrar.AddAction(Spawner);
}

// ----------------------------------------------------------------
//	[AnimState] 애니메이션 상태 노드
// ----------------------------------------------------------------

IMPLEMENT_CLASS(UK2Node_AnimState, UK2Node)

UK2Node_AnimState::UK2Node_AnimState()
{
    TitleColor = ImColor(200, 100, 100);
}

void UK2Node_AnimState::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    UK2Node::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        FJsonSerializer::ReadString(InOutHandle, "StateName", StateName);
    }
    else
    {
        InOutHandle["StateName"] = StateName;
    }
}

void UK2Node_AnimState::AllocateDefaultPins()
{
    // 상태 머신 그래프의 흐름(Flow)을 위한 Exec 핀
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Exec, "Enter");
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Exec, "Exit");

    // FAnimationState의 멤버에 해당하는 핀들
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::AnimSequence, "Animation");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Bool, "Looping", "false");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "PlayRate", "1.0");
}

void UK2Node_AnimState::RenderBody()
{
    ImGui::PushItemWidth(150.0f);
    ImGui::InputText("상태 이름", &StateName);
    ImGui::PopItemWidth();
}

void UK2Node_AnimState::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());

    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();

    ActionRegistrar.AddAction(Spawner);
}

// ----------------------------------------------------------------
//	[AnimTransition] 애니메이션 전이 노드
// ----------------------------------------------------------------

IMPLEMENT_CLASS(UK2Node_AnimTransition, UK2Node)

UK2Node_AnimTransition::UK2Node_AnimTransition()
{
    TitleColor = ImColor(100, 100, 200);
}

void UK2Node_AnimTransition::AllocateDefaultPins()
{
    // 상태 머신 그래프의 흐름(Flow)을 위한 Exec 핀
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Exec, "Execute"); 
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Exec, "Transition To");
    
    // FStateTransition의 멤버에 해당하는 핀들
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Bool, "Can Transition");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "Blend Time");
}

void UK2Node_AnimTransition::RenderBody()
{
}

void UK2Node_AnimTransition::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());

    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();

    ActionRegistrar.AddAction(Spawner);
}

// ----------------------------------------------------------------
//	[BlendSpace1D] 1D 블렌드 스페이스 노드
// ----------------------------------------------------------------

IMPLEMENT_CLASS(UK2Node_BlendSpace1D, UK2Node)

UK2Node_BlendSpace1D::UK2Node_BlendSpace1D()
{
    TitleColor = ImColor(100, 200, 100);

    // 기본 3개 샘플 슬롯 생성
    SampleAnimations.SetNum(3);
    SamplePositions.SetNum(3);
    SamplePositions[0] = 0.0f;
    SamplePositions[1] = 100.0f;
    SamplePositions[2] = 200.0f;

    // BlendSpace 객체 생성
    BlendSpace = NewObject<UBlendSpace1D>();
}

void UK2Node_BlendSpace1D::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    UK2Node::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        // 샘플 개수 로드
        int32 NumSamples = 3;
        FJsonSerializer::ReadInt32(InOutHandle, "NumSamples", NumSamples);

        SampleAnimations.SetNum(NumSamples);
        SamplePositions.SetNum(NumSamples);

        // 각 샘플 로드
        for (int32 i = 0; i < NumSamples; ++i)
        {
            FString AnimKey = "SampleAnim_" + std::to_string(i);
            FString PosKey = "SamplePos_" + std::to_string(i);

            FString AnimPath;
            if (FJsonSerializer::ReadString(InOutHandle, AnimKey, AnimPath) && !AnimPath.empty())
            {
                SampleAnimations[i] = RESOURCE.Get<UAnimSequence>(AnimPath);
            }

            FJsonSerializer::ReadFloat(InOutHandle, PosKey, SamplePositions[i]);
        }

        RebuildBlendSpace();
    }
    else
    {
        // 샘플 개수 저장
        InOutHandle["NumSamples"] = SampleAnimations.Num();

        // 각 샘플 저장
        for (int32 i = 0; i < SampleAnimations.Num(); ++i)
        {
            FString AnimKey = "SampleAnim_" + std::to_string(i);
            FString PosKey = "SamplePos_" + std::to_string(i);

            if (SampleAnimations[i])
            {
                InOutHandle[AnimKey] = SampleAnimations[i]->GetFilePath();
            }
            InOutHandle[PosKey] = SamplePositions[i];
        }
    }
}

void UK2Node_BlendSpace1D::AllocateDefaultPins()
{
    // 입력: 파라미터 (예: Speed)
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "Parameter", "0.0");

    // 출력: 블렌드된 포즈
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::AnimSequence, "Output");
}

void UK2Node_BlendSpace1D::RenderBody()
{
    ImGui::PushItemWidth(200.0f);

    // 샘플 개수 조정
    int32 NumSamples = SampleAnimations.Num();
    if (ImGui::InputInt("Samples", &NumSamples))
    {
        NumSamples = FMath::Clamp(NumSamples, 1, 10);
        SampleAnimations.SetNum(NumSamples);
        SamplePositions.SetNum(NumSamples);

        // 새 샘플의 기본 Position 설정
        for (int32 i = 0; i < NumSamples; ++i)
        {
            if (SamplePositions[i] == 0.0f && i > 0)
            {
                SamplePositions[i] = i * 100.0f;
            }
        }
    }

    ImGui::Separator();

    // 각 샘플 편집
    bool bNeedRebuild = false;

    for (int32 i = 0; i < SampleAnimations.Num(); ++i)
    {
        ImGui::PushID(i);

        // Position 슬라이더
        FString PosLabel = "Pos " + std::to_string(i);
        if (ImGui::DragFloat(PosLabel.c_str(), &SamplePositions[i], 1.0f, 0.0f, 500.0f))
        {
            bNeedRebuild = true;
        }

        // 애니메이션 선택 버튼
        FString AnimName = SampleAnimations[i] ? SampleAnimations[i]->GetFilePath() : "None";
        FString ButtonLabel = AnimName + "##Anim" + std::to_string(i);

        if (ImGui::Button(ButtonLabel.c_str(), ImVec2(180, 0)))
        {
            FString PopupID = "##AnimPopup_" + std::to_string(NodeID) + "_" + std::to_string(i);
            ed::Suspend();
            ImGui::OpenPopup(PopupID.c_str());
            ed::Resume();
        }

        // 애니메이션 선택 팝업
        FString PopupID = "##AnimPopup_" + std::to_string(NodeID) + "_" + std::to_string(i);
        if (ImGui::IsPopupOpen(PopupID.c_str()))
        {
            ed::Suspend();
            if (ImGui::BeginPopup(PopupID.c_str()))
            {
                TArray<UAnimSequence*> AnimSequences = RESOURCE.GetAll<UAnimSequence>();

                for (UAnimSequence* Anim : AnimSequences)
                {
                    if (!Anim) continue;

                    const FString AssetName = Anim->GetFilePath();
                    bool bIsSelected = (SampleAnimations[i] == Anim);

                    if (ImGui::Selectable(AssetName.c_str(), bIsSelected))
                    {
                        SampleAnimations[i] = Anim;
                        bNeedRebuild = true;
                        ImGui::CloseCurrentPopup();
                    }
                }

                ImGui::EndPopup();
            }
            ed::Resume();
        }

        ImGui::PopID();
    }

    if (bNeedRebuild)
    {
        RebuildBlendSpace();
    }

    ImGui::PopItemWidth();
}

FBlueprintValue UK2Node_BlendSpace1D::EvaluatePin(const UEdGraphPin* OutputPin, FBlueprintContext* Context)
{
    if (OutputPin->PinName == "Output")
    {
        float Parameter = FBlueprintEvaluator::EvaluateInput<float>(FindPin("Parameter"), Context);

        if (BlendSpace)
        {
            BlendSpace->SetParameter(Parameter);
        }
        return BlendSpace;
    }
    return static_cast<UBlendSpace1D*>(nullptr);
}

void UK2Node_BlendSpace1D::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());

    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();

    ActionRegistrar.AddAction(Spawner);
}

void UK2Node_BlendSpace1D::RebuildBlendSpace()
{
    if (!BlendSpace)
    {
        BlendSpace = NewObject<UBlendSpace1D>();
    }

    BlendSpace->ClearSamples();

    for (int32 i = 0; i < SampleAnimations.Num(); ++i)
    {
        if (SampleAnimations[i])
        {
            BlendSpace->AddSample(SampleAnimations[i], SamplePositions[i]);
        }
    }
}