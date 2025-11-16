#pragma once

#include "K2Node.h"

// ----------------------------------------------------------------
//	[Float] 노드
// ----------------------------------------------------------------

UCLASS(DisplayName="UK2Node_Add_FloatFloat", Description="블루프린트 float 덧셈 노드")
class UK2Node_Add_FloatFloat : public UK2Node
{
    DECLARE_CLASS(UK2Node_Add_FloatFloat, UK2Node);

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "+ (Float)"; }
    virtual FString GetMenuCategory() const override { return "수학"; }
    virtual bool IsNodePure() const override { return true; }
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;
    
    // --- UK2Node 인터페이스 ---
public:
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};

UCLASS(DisplayName="UK2Node_Subtract_FloatFloat", Description="블루프린트 float 뺄셈 노드")
class UK2Node_Subtract_FloatFloat : public UK2Node
{
    DECLARE_CLASS(UK2Node_Subtract_FloatFloat, UK2Node);

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "- (Float)"; }
    virtual FString GetMenuCategory() const override { return "수학"; }
    virtual bool IsNodePure() const override { return true; }
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;

    // --- UK2Node 인터페이스 ---
public:
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};

UCLASS(DisplayName="UK2Node_Multiply_FloatFloat", Description="블루프린트 float 곱셈 노드")
class UK2Node_Multiply_FloatFloat : public UK2Node
{
    DECLARE_CLASS(UK2Node_Multiply_FloatFloat, UK2Node);

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "* (Float)"; }
    virtual FString GetMenuCategory() const override { return "수학"; }
    virtual bool IsNodePure() const override { return true; }
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;
    
    // --- UK2Node 인터페이스 ---
public:
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};

UCLASS(DisplayName = "UK2Node_Divide_FloatFloat", Description = "블루프린트 float 나눗셈 노드")
class UK2Node_Divide_FloatFloat : public UK2Node
{
    DECLARE_CLASS(UK2Node_Divide_FloatFloat, UK2Node);

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "/ (Float)"; }
    virtual FString GetMenuCategory() const override { return "수학"; }
    virtual bool IsNodePure() const override { return true; }
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;

    // --- UK2Node 인터페이스 ---
public:
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};

UCLASS(DisplayName = "UK2Node_Greater_FloatFloat", Description = "블루프린트 float '>' 노드")
class UK2Node_Greater_FloatFloat : public UK2Node
{
    DECLARE_CLASS(UK2Node_Greater_FloatFloat, UK2Node);

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "> (Float)"; }
    virtual FString GetMenuCategory() const override { return "수학"; }
    virtual bool IsNodePure() const override { return true; }
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;

    // --- UK2Node 인터페이스 ---
public:
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};

UCLASS(DisplayName = "UK2Node_Equal_FloatFloat", Description = "블루프린트 float '==' 노드")
class UK2Node_Equal_FloatFloat : public UK2Node
{
    DECLARE_CLASS(UK2Node_Equal_FloatFloat, UK2Node);

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "== (Float)"; }
    virtual FString GetMenuCategory() const override { return "수학"; }
    virtual bool IsNodePure() const override { return true; }
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;

    // --- UK2Node 인터페이스 ---
public:
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};

// ----------------------------------------------------------------
//	[Int] 노드
// ----------------------------------------------------------------

UCLASS(DisplayName = "UK2Node_Add_IntInt", Description = "블루프린트 int 덧셈 노드")
class UK2Node_Add_IntInt : public UK2Node
{
    DECLARE_CLASS(UK2Node_Add_IntInt, UK2Node);

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "+ (Int)"; }
    virtual FString GetMenuCategory() const override { return "수학"; }
    virtual bool IsNodePure() const override { return true; }
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;

    // --- UK2Node 인터페이스 ---
public:
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};

UCLASS(DisplayName = "UK2Node_Subtract_IntInt", Description = "블루프린트 int 뺄셈 노드")
class UK2Node_Subtract_IntInt : public UK2Node
{
    DECLARE_CLASS(UK2Node_Subtract_IntInt, UK2Node);

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "- (Int)"; }
    virtual FString GetMenuCategory() const override { return "수학"; }
    virtual bool IsNodePure() const override { return true; }
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;

    // --- UK2Node 인터페이스 ---
public:
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};

UCLASS(DisplayName = "UK2Node_Multiply_IntInt", Description = "블루프린트 int 곱셈 노드")
class UK2Node_Multiply_IntInt : public UK2Node
{
    DECLARE_CLASS(UK2Node_Multiply_IntInt, UK2Node);

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "* (Int)"; }
    virtual FString GetMenuCategory() const override { return "수학"; }
    virtual bool IsNodePure() const override { return true; }
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;

    // --- UK2Node 인터페이스 ---
public:
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};

UCLASS(DisplayName = "UK2Node_Divide_IntInt", Description = "블루프린트 int 나눗셈 노드")
class UK2Node_Divide_IntInt : public UK2Node
{
    DECLARE_CLASS(UK2Node_Divide_IntInt, UK2Node);

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "/ (Int)"; }
    virtual FString GetMenuCategory() const override { return "수학"; }
    virtual bool IsNodePure() const override { return true; }
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;

    // --- UK2Node 인터페이스 ---
public:
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};

UCLASS(DisplayName = "UK2Node_Greater_IntInt", Description = "블루프린트 int '>' 노드")
class UK2Node_Greater_IntInt : public UK2Node
{
    DECLARE_CLASS(UK2Node_Greater_IntInt, UK2Node);

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "> (Int)"; }
    virtual FString GetMenuCategory() const override { return "수학"; }
    virtual bool IsNodePure() const override { return true; }
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;

    // --- UK2Node 인터페이스 ---
public:
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};

UCLASS(DisplayName = "UK2Node_Equal_IntInt", Description = "블루프린트 int '==' 노드")
class UK2Node_Equal_IntInt : public UK2Node
{
    DECLARE_CLASS(UK2Node_Equal_IntInt, UK2Node);

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "== (Int)"; }
    virtual FString GetMenuCategory() const override { return "수학"; }
    virtual bool IsNodePure() const override { return true; }
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;

    // --- UK2Node 인터페이스 ---
public:
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};