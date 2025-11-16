#pragma once

#include "K2Node.h"

// ----------------------------------------------------------------
//	[Input] 키보드 입력 확인 노드
// ----------------------------------------------------------------

UCLASS(DisplayName="UK2Node_IsPressed", Description="지정된 키가 현재 눌려있는지 확인합니다 (bool 반환).")
class UK2Node_IsPressed : public UK2Node
{
    DECLARE_CLASS(UK2Node_IsPressed, UK2Node);

public:
    /** 확인할 키의 이름 (예: "Space", "W") */
    FString KeyName = "Space";
    
    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "Is Key Pressed"; }
    virtual FString GetMenuCategory() const override { return "입력"; };
    virtual bool IsNodePure() const override { return true; } // 순수 노드로 처리
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;
    
    // --- UK2Node 인터페이스 ---
public:
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};

// ----------------------------------------------------------------
//	[Input] 마우스 위치 확인 노드
// ----------------------------------------------------------------

UCLASS(DisplayName = "UK2Node_GetMousePosition", Description = "현재 마우스 커서의 위치(X, Y)를 반환합니다.")
class UK2Node_GetMousePosition : public UK2Node
{
    DECLARE_CLASS(UK2Node_GetMousePosition, UK2Node);

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "Get Mouse Position"; }
    virtual FString GetMenuCategory() const override { return "입력"; };
    virtual bool IsNodePure() const override { return true; } // 순수 노드로 처리
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override; 

    // --- UK2Node 인터페이스 ---
public:
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};