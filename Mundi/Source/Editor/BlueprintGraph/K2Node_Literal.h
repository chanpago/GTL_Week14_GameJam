#pragma once

#include "K2Node.h"

// ----------------------------------------------------------------
//	[Int] 리터럴 노드
// ----------------------------------------------------------------

UCLASS(DisplayName="UK2Node_Literal_Int", Description="블루프린트 int 리터럴 노드")
class UK2Node_Literal_Int : public UK2Node
{
    DECLARE_CLASS(UK2Node_Literal_Int, UK2Node);

public:
    /** @todo UPROPERTY 시스템 통합 */
    int32 Value;
    
    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "int"; }
    virtual FString GetMenuCategory() const override { return "리터럴"; };
    virtual bool IsNodePure() const override { return true; }
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;
    
    // --- UK2Node 인터페이스 ---
public:
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};

// ----------------------------------------------------------------
//	[Float] 리터럴 노드
// ----------------------------------------------------------------

UCLASS(DisplayName="UK2Node_Literal_Float", Description="블루프린트 float 리터럴 노드")
class UK2Node_Literal_Float : public UK2Node
{
    DECLARE_CLASS(UK2Node_Literal_Float, UK2Node);

public:
    /** @todo UPROPERTY 시스템 통합 */
    float Value;
    
    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "float"; }
    virtual FString GetMenuCategory() const override { return "리터럴"; };
    virtual bool IsNodePure() const override { return true; }
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;
    
    // --- UK2Node 인터페이스 ---
public:
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};