#pragma once

#include "K2Node.h"

// ----------------------------------------------------------------
//	[AnimSequence] 애니메이션 시퀀스 노드 
// ----------------------------------------------------------------

UCLASS(DisplayName = "UK2Node_AnimSequence", Description = "블루프린트 AnimSequence 리터럴 노드")
class UK2Node_AnimSequence : public UK2Node
{
    DECLARE_CLASS(UK2Node_AnimSequence, UK2Node);

public:
    /** @todo UPROPERTY 시스템 통합 */
    UAnimSequence* Value = nullptr;

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "애니메이션 시퀀스"; }
    virtual FString GetMenuCategory() const override { return "Animation"; };
    virtual bool IsNodePure() const override { return true; }
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;

    // --- UK2Node 인터페이스 ---
public:
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};

// ----------------------------------------------------------------
//	[AnimState] 애니메이션 상태 노드
// ----------------------------------------------------------------

UCLASS(DisplayName = "UK2Node_AnimState", Description = "애니메이션 상태 머신(ASM)의 상태를 정의합니다.")
class UK2Node_AnimState : public UK2Node
{
    DECLARE_CLASS(UK2Node_AnimState, UK2Node);

public:
    /** 상태의 고유 이름. FAnimationState::Name에 해당합니다. */
    FString StateName = "NewState";

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "애니메이션 스테이트"; }
    virtual FString GetMenuCategory() const override { return "Animation"; };
    virtual bool IsNodePure() const override { return false; } // 실행 흐름(Exec)이 있으므로 Pure가 아님
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;

    // --- UK2Node 인터페이스 ---
public:
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};

// ----------------------------------------------------------------
//	[AnimTransition] 애니메이션 전이 노드
// ----------------------------------------------------------------

UCLASS(DisplayName = "UK2Node_AnimTransition", Description = "애니메이션 상태 간의 전이(규칙)를 정의합니다.")
class UK2Node_AnimTransition : public UK2Node
{
    DECLARE_CLASS(UK2Node_AnimTransition, UK2Node);

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "애니메이션 상태 전이"; }
    virtual FString GetMenuCategory() const override { return "Animation"; };
    virtual bool IsNodePure() const override { return false; } // 실행 흐름(Exec)이 있으므로 Pure가 아님
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override; 

    // --- UK2Node 인터페이스 ---
public:
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};