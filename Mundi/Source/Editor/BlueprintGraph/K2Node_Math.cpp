#include "pch.h"
#include "K2Node_Math.h"
#include "BlueprintActionDatabase.h"

// ----------------------------------------------------------------
//  [Float] 노드
// ----------------------------------------------------------------

IMPLEMENT_CLASS(UK2Node_Add_FloatFloat, UK2Node)

void UK2Node_Add_FloatFloat::AllocateDefaultPins()
{
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "A");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "B");
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Float, "Result");
    TitleColor = ImColor(147, 226, 74); 
}

void UK2Node_Add_FloatFloat::RenderBody()
{
}

void UK2Node_Add_FloatFloat::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();
    ActionRegistrar.AddAction(Spawner);
}

IMPLEMENT_CLASS(UK2Node_Subtract_FloatFloat, UK2Node)

void UK2Node_Subtract_FloatFloat::AllocateDefaultPins()
{
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "A");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "B");
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Float, "Result");
    TitleColor = ImColor(147, 226, 74);
}

void UK2Node_Subtract_FloatFloat::RenderBody()
{
}

void UK2Node_Subtract_FloatFloat::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();
    ActionRegistrar.AddAction(Spawner);
}

IMPLEMENT_CLASS(UK2Node_Multiply_FloatFloat, UK2Node)

void UK2Node_Multiply_FloatFloat::AllocateDefaultPins()
{
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "A");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "B");
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Float, "Result");
    TitleColor = ImColor(147, 226, 74);
}

void UK2Node_Multiply_FloatFloat::RenderBody()
{
}

void UK2Node_Multiply_FloatFloat::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();
    ActionRegistrar.AddAction(Spawner);
}

IMPLEMENT_CLASS(UK2Node_Divide_FloatFloat, UK2Node)

void UK2Node_Divide_FloatFloat::AllocateDefaultPins()
{
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "A");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "B");
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Float, "Result");
    TitleColor = ImColor(147, 226, 74);
}

void UK2Node_Divide_FloatFloat::RenderBody()
{
}

void UK2Node_Divide_FloatFloat::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();
    ActionRegistrar.AddAction(Spawner);
}

IMPLEMENT_CLASS(UK2Node_Greater_FloatFloat, UK2Node)

void UK2Node_Greater_FloatFloat::AllocateDefaultPins()
{
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "A");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "B");
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Bool, "Result");
    TitleColor = ImColor(220, 48, 48); // Bool 핀 색상 (가정)
}

void UK2Node_Greater_FloatFloat::RenderBody()
{
}

void UK2Node_Greater_FloatFloat::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();
    ActionRegistrar.AddAction(Spawner);
}

IMPLEMENT_CLASS(UK2Node_Equal_FloatFloat, UK2Node)

void UK2Node_Equal_FloatFloat::AllocateDefaultPins()
{
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "A");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Float, "B");
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Bool, "Result");
    TitleColor = ImColor(220, 48, 48); // Bool 핀 색상 (가정)
}

void UK2Node_Equal_FloatFloat::RenderBody()
{
}

void UK2Node_Equal_FloatFloat::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();
    ActionRegistrar.AddAction(Spawner);
}

// ----------------------------------------------------------------
//  [Int] 노드
// ----------------------------------------------------------------

IMPLEMENT_CLASS(UK2Node_Add_IntInt, UK2Node)

void UK2Node_Add_IntInt::AllocateDefaultPins()
{
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Int, "A");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Int, "B");
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Int, "Result");
    TitleColor = ImColor(68, 201, 156); 
}

void UK2Node_Add_IntInt::RenderBody()
{
}

void UK2Node_Add_IntInt::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();
    ActionRegistrar.AddAction(Spawner);
}

IMPLEMENT_CLASS(UK2Node_Subtract_IntInt, UK2Node)

void UK2Node_Subtract_IntInt::AllocateDefaultPins()
{
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Int, "A");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Int, "B");
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Int, "Result");
    TitleColor = ImColor(68, 201, 156);
}

void UK2Node_Subtract_IntInt::RenderBody()
{
}

void UK2Node_Subtract_IntInt::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();
    ActionRegistrar.AddAction(Spawner);
}

IMPLEMENT_CLASS(UK2Node_Multiply_IntInt, UK2Node)

void UK2Node_Multiply_IntInt::AllocateDefaultPins()
{
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Int, "A");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Int, "B");
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Int, "Result");
    TitleColor = ImColor(68, 201, 156);
}

void UK2Node_Multiply_IntInt::RenderBody()
{
}

void UK2Node_Multiply_IntInt::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();
    ActionRegistrar.AddAction(Spawner);
}

IMPLEMENT_CLASS(UK2Node_Divide_IntInt, UK2Node)

void UK2Node_Divide_IntInt::AllocateDefaultPins()
{
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Int, "A");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Int, "B");
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Int, "Result");
    TitleColor = ImColor(68, 201, 156);
}

void UK2Node_Divide_IntInt::RenderBody()
{
}

void UK2Node_Divide_IntInt::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();
    ActionRegistrar.AddAction(Spawner);
}

IMPLEMENT_CLASS(UK2Node_Greater_IntInt, UK2Node)

void UK2Node_Greater_IntInt::AllocateDefaultPins()
{
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Int, "A");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Int, "B");
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Bool, "Result");
    TitleColor = ImColor(220, 48, 48); // Bool 핀 색상 (가정)
}

void UK2Node_Greater_IntInt::RenderBody()
{
}

void UK2Node_Greater_IntInt::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();
    ActionRegistrar.AddAction(Spawner);
}

IMPLEMENT_CLASS(UK2Node_Equal_IntInt, UK2Node)

void UK2Node_Equal_IntInt::AllocateDefaultPins()
{
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Int, "A");
    CreatePin(EEdGraphPinDirection::EGPD_Input, FEdGraphPinCategory::Int, "B");
    CreatePin(EEdGraphPinDirection::EGPD_Output, FEdGraphPinCategory::Bool, "Result");
    TitleColor = ImColor(220, 48, 48); // Bool 핀 색상 (가정)
}

void UK2Node_Equal_IntInt::RenderBody()
{
}

void UK2Node_Equal_IntInt::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
    UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(GetClass());
    Spawner->MenuName = GetNodeTitle();
    Spawner->Category = GetMenuCategory();
    ActionRegistrar.AddAction(Spawner);
}
