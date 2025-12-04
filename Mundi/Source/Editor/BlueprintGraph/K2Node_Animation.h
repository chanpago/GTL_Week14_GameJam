/**
* ===========================================================================
 * @file      K2Node_Animation.h
 * @author    geb0598
 * @date      2025/11/17
 * @brief     블루프린트에서 사용되는 애니메이션 관련 노드(애니메이션 시퀀스, 상태 머신 등)를 정의한다.
 *
 * ===========================================================================
 *
 * @note 코딩 규칙
 *   - `UK2Node_AnimSequence`: 애니메이션 시퀀스 애셋을 값으로 가지는 리터럴 노드. 출력 핀 이름은 'Value'로 지정한다.
 *   - `UK2Node_AnimStateEntry`: 상태 머신의 진입점. 사용자가 직접 생성하지 않는다.
 *   - `UK2Node_AnimState`: 상태 머신의 상태를 나타낸다. 'StateName' 프로퍼티로 상태 이름을 지정한다.
 *   - `UK2Node_AnimTransition`: 상태 간의 전이를 정의하는 노드.
 *   - 모든 애니메이션 노드는 "애니메이션" 메뉴 카테고리에 속한다.
 *
 * ===========================================================================
 */

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
    UK2Node_AnimSequence();

    /** @todo UPROPERTY 시스템 통합 */
    UAnimSequence* Value = nullptr;

    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "Animation Sequence"; }
    virtual bool IsNodePure() const override { return true; }
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;
    virtual FBlueprintValue EvaluatePin(const UEdGraphPin* OutputPin, FBlueprintContext* Context) override;

    // --- UK2Node 인터페이스 ---
public:
    virtual FString GetMenuCategory() const override { return "애니메이션"; };
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};

// ----------------------------------------------------------------
//	[AnimStateEntry] 애니메이션 상태 머신 진입점
// ----------------------------------------------------------------

UCLASS(DisplayName = "UK2Node_AnimStateEntry", Description = "애니메이션 상태 머신의 진입점 노드입니다.")
class UK2Node_AnimStateEntry : public UK2Node
{
    DECLARE_CLASS(UK2Node_AnimStateEntry, UK2Node);

public:
    UK2Node_AnimStateEntry();

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "Entry"; }
    virtual bool IsNodePure() const override { return false; }
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;

    // --- UK2Node 인터페이스 ---
public:
    virtual FString GetMenuCategory() const override { return "애니메이션"; };
    /**
     * @note 이 노드는 사용자가 메뉴에서 직접 스폰(spawn)하는 것이 아니라, 에디터를 실행 시 최초 한 번만 생성해야 한다.
     *       그러나, 현재는 디버깅용으로 스폰을 가능하게 설정해둔다.
     * @todo 애니메이션 그래프 편집기 제작이 완료되면 GetMenuActions의 구현부를 지운다.
     */
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
    UK2Node_AnimState();

    /** 상태의 고유 이름. FAnimationState::Name에 해당한다. */
    FString StateName = "NewState";
    
    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "Animation State"; }
    virtual bool IsNodePure() const override { return false; } // 실행 흐름(Exec)이 있으므로 Pure가 아님
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;

    // --- UK2Node 인터페이스 ---
public:
    virtual FString GetMenuCategory() const override { return "애니메이션"; };
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};

// ----------------------------------------------------------------
//	[AnimTransition] 애니메이션 전이 노드
// ----------------------------------------------------------------

UCLASS(DisplayName = "UK2Node_AnimTransition", Description = "애니메이션 상태 간의 전이(규칙)를 정의합니다.")
class UK2Node_AnimTransition : public UK2Node
{
    DECLARE_CLASS(UK2Node_AnimTransition, UK2Node);

public:
    UK2Node_AnimTransition();

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "Animation Transition"; }
    virtual bool IsNodePure() const override { return false; } // 실행 흐름(Exec)이 있으므로 Pure가 아님
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;

    // --- UK2Node 인터페이스 ---
public:
    virtual FString GetMenuCategory() const override { return "애니메이션"; };
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};

// ----------------------------------------------------------------
//	[BlendSpace1D] 1D 블렌드 스페이스 노드
// ----------------------------------------------------------------

class UBlendSpace1D;

UCLASS(DisplayName = "UK2Node_BlendSpace1D", Description = "1D 블렌드 스페이스를 정의합니다. 파라미터에 따라 여러 애니메이션을 블렌딩합니다.")
class UK2Node_BlendSpace1D : public UK2Node
{
    DECLARE_CLASS(UK2Node_BlendSpace1D, UK2Node);

public:
    UK2Node_BlendSpace1D();

    /** 블렌드 스페이스 데이터 */
    UBlendSpace1D* BlendSpace = nullptr;

    /** 블렌드 스페이스 패러미터 범위 */
    float MinRange = 0.0f;
    float MaxRange = 100.0f;

    /** 샘플 애니메이션들 (UI용) */
    TArray<UAnimSequence*> SampleAnimations;
    TArray<float> SamplePositions;

    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "Blend Space 1D"; }
    virtual bool IsNodePure() const override { return true; }
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;
    virtual FBlueprintValue EvaluatePin(const UEdGraphPin* OutputPin, FBlueprintContext* Context) override;

    // --- UK2Node 인터페이스 ---
public:
    virtual FString GetMenuCategory() const override { return "애니메이션"; };
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

private:
    void RebuildBlendSpace();
};

// ----------------------------------------------------------------
//	[BlendSpace2D] 2D 블렌드 스페이스 노드
// ----------------------------------------------------------------

class UBlendSpace2D;

UCLASS(DisplayName = "UK2Node_BlendSpace2D", Description = "2D 블렌드 스페이스를 정의합니다. X(Direction), Y(Speed) 파라미터에 따라 여러 애니메이션을 블렌딩합니다.")
class UK2Node_BlendSpace2D : public UK2Node
{
    DECLARE_CLASS(UK2Node_BlendSpace2D, UK2Node);

public:
    UK2Node_BlendSpace2D();

    /** 블렌드 스페이스 데이터 */
    UBlendSpace2D* BlendSpace = nullptr;

    /** X축 (Direction) 범위 */
    float MinX = -100.0f;
    float MaxX = 100.0f;

    /** Y축 (Speed) 범위 */
    float MinY = -100.0f;
    float MaxY = 100.0f;

    /** 샘플 애니메이션들 (UI용) */
    TArray<UAnimSequence*> SampleAnimations;
    TArray<FVector2D> SamplePositions;

    /** 수동 삼각형 인덱스 (UI용, A*1000000 + B*1000 + C 형태로 저장) */
    TArray<int32> TriangleIndicesA;
    TArray<int32> TriangleIndicesB;
    TArray<int32> TriangleIndicesC;

    /** 현재 선택된 점 인덱스 (삼각형 생성용) */
    TArray<int32> SelectedIndices;

    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "Blend Space 2D"; }
    virtual bool IsNodePure() const override { return true; }
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;
    virtual FBlueprintValue EvaluatePin(const UEdGraphPin* OutputPin, FBlueprintContext* Context) override;

    // --- UK2Node 인터페이스 ---
public:
    virtual FString GetMenuCategory() const override { return "애니메이션"; };
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;

    /** UI 데이터를 BlendSpace 객체에 동기화 */
    void RebuildBlendSpace();

private:
    void DrawTriangles(ImDrawList* DrawList, ImVec2 CanvasMin, float CanvasSize);
};

// ----------------------------------------------------------------
//	[IsAnimationFinished] 애니메이션 종료 여부 확인 노드
// ----------------------------------------------------------------

UCLASS(DisplayName = "UK2Node_IsAnimationFinished", Description = "현재 애니메이션이 끝났는지 확인합니다. 루핑이 아닌 애니메이션이 끝까지 재생되면 true를 반환합니다.")
class UK2Node_IsAnimationFinished : public UK2Node
{
    DECLARE_CLASS(UK2Node_IsAnimationFinished, UK2Node);

public:
    UK2Node_IsAnimationFinished();

    // --- UEdGraphNode 인터페이스 ---
public:
    virtual FString GetNodeTitle() const override { return "Is Animation Finished"; }
    virtual bool IsNodePure() const override { return true; }
    virtual void AllocateDefaultPins() override;
    virtual void RenderBody() override;
    virtual FBlueprintValue EvaluatePin(const UEdGraphPin* OutputPin, FBlueprintContext* Context) override;

    // --- UK2Node 인터페이스 ---
public:
    virtual FString GetMenuCategory() const override { return "애니메이션"; };
    virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
};