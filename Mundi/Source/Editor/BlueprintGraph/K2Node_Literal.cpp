#include "pch.h"
#include "imgui-node-editor/imgui_node_editor.h"
#include "K2Node_Literal.h"
#include "BlueprintActionDatabase.h"

namespace ed = ax::NodeEditor;

// ----------------------------------------------------------------
//	[Int] 리터럴 노드
// ----------------------------------------------------------------

IMPLEMENT_CLASS(UK2Node_Literal_Int, UK2Node)

void UK2Node_Literal_Int::AllocateDefaultPins()
{
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Int, "Value");
}

void UK2Node_Literal_Int::RenderBody()
{
    ImGui::PushItemWidth(100.0f);
    ImGui::DragInt("##value", &Value, 0.1f);
    ImGui::PopItemWidth(); 
}

void UK2Node_Literal_Int::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());

    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();

    ActionRegistrar.AddAction(Spawner);
}

// ----------------------------------------------------------------
//	[Float] 리터럴 노드
// ----------------------------------------------------------------

IMPLEMENT_CLASS(UK2Node_Literal_Float, UK2Node)

void UK2Node_Literal_Float::AllocateDefaultPins()
{
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Float, "Value");
}

void UK2Node_Literal_Float::RenderBody()
{
    ImGui::PushItemWidth(100.0f);
    ImGui::DragFloat("##value", &Value, 0.01f);
    ImGui::PopItemWidth();

}

void UK2Node_Literal_Float::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());

    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();

    ActionRegistrar.AddAction(Spawner);
}