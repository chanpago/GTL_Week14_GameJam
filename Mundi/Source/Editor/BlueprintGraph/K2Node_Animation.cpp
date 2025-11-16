#include "pch.h"
#include "ImGui/imgui_stdlib.h"
#include "imgui-node-editor/imgui_node_editor.h"
#include "K2Node_Animation.h"
#include "BlueprintActionDatabase.h"

namespace ed = ax::NodeEditor;

// ----------------------------------------------------------------
//	[AnimSequence] 애니메이션 시퀀스 노드 (K2Node_Literal.cpp에서 이동)
// ----------------------------------------------------------------

IMPLEMENT_CLASS(UK2Node_AnimSequence, UK2Node)

void UK2Node_AnimSequence::AllocateDefaultPins()
{
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::AnimSequence, "Value");

    // AnimSequence 핀 색상 (예: 파란색 계열)
    TitleColor = ImColor(100, 120, 255);
}

/**
 * @note bOpenPopup과 ed::Suspend를 활용하지 않으면, 스크린 공간이 아닌 캔버스 공간 좌표가 사용되어서
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

    bool bOpenPopup = false;

    if (ImGui::Button(PreviewName.c_str()))
    {
        bOpenPopup = true;
    }

    ed::Suspend();

    if (bOpenPopup)
    {
        ImGui::OpenPopup(PopupID.c_str());
    }

    if (ImGui::BeginPopup(PopupID.c_str()))
    {
        TArray<UAnimSequence*> AnimSequences = RESOURCE.GetAll<UAnimSequence>();

        if (ImGui::Selectable("None", Value == nullptr))
        {
            Value = nullptr;
            ImGui::CloseCurrentPopup();
        }

        for (UAnimSequence* Anim : AnimSequences)
        {
            if (!Anim) continue;

            const FString AssetName = Anim->GetFilePath();
            bool bIsSelected = (Value == Anim);

            if (ImGui::Selectable(AssetName.c_str(), bIsSelected))
            {
                Value = Anim;
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    ed::Resume();
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

void UK2Node_AnimState::AllocateDefaultPins()
{
    // FAnimationState의 멤버에 해당하는 핀들
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::AnimSequence, "Animation");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Bool, "Looping");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "PlayRate");

    // 상태 머신 그래프의 흐름(Flow)을 위한 Exec 핀
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Exec, "Enter");
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Exec, "Exit");

    TitleColor = ImColor(200, 100, 100); 
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

void UK2Node_AnimTransition::AllocateDefaultPins()
{
    // FStateTransition의 멤버에 해당하는 핀들
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Bool, "Can Transition");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "Blend Time");

    // 상태 머신 그래프의 흐름(Flow)을 위한 Exec 핀
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Exec, "Execute"); 
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Exec, "Transition To"); 

    TitleColor = ImColor(100, 100, 200);
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