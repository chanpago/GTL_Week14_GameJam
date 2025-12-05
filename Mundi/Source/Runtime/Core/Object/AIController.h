#pragma once

#include "Controller.h"
#include "AAIController.generated.h"

class APawn;

// ============================================================================
// AAIController - AI 컨트롤러 베이스 클래스
// ============================================================================
// 사용법:
//   1. AAIController를 상속받아 적 유형별 컨트롤러 생성
//   2. Possess()로 적 폰에 빙의
//   3. UpdateAI()에서 AI 로직 구현
// ============================================================================

UCLASS(DisplayName = "AI 컨트롤러", Description = "AI 컨트롤러 베이스 클래스")
class AAIController : public AController
{
public:
    GENERATED_REFLECTION_BODY()

    AAIController();
    virtual ~AAIController() override = default;

    // ========================================================================
    // Lifecycle
    // ========================================================================
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    // ========================================================================
    // AI 제어
    // ========================================================================

    /** AI 활성화/비활성화 */
    void SetAIEnabled(bool bEnabled) { bAIEnabled = bEnabled; }
    bool IsAIEnabled() const { return bAIEnabled; }

    /** 타겟 설정 */
    void SetTargetActor(AActor* InTarget) { TargetActor = InTarget; }
    AActor* GetTargetActor() const { return TargetActor; }

    /** 타겟까지 이동 */
    virtual void MoveToActor(AActor* Goal, float AcceptanceRadius = 100.f);
    virtual void MoveToLocation(const FVector& Location, float AcceptanceRadius = 100.f);
    virtual void StopMovement();

    /** 타겟 방향으로 회전 */
    virtual void FocusOnActor(AActor* Actor);
    virtual void FocusOnLocation(const FVector& Location);

    /** 거리 계산 */
    float GetDistanceToActor(AActor* Actor) const;
    float GetDistanceToLocation(const FVector& Location) const;

    /** 이동 상태 확인 */
    bool IsMoving() const { return bIsMoving; }
    bool HasReachedDestination() const;

protected:
    // ========================================================================
    // AI 로직 (서브클래스에서 오버라이드)
    // ========================================================================
    virtual void UpdateAI(float DeltaTime);
    virtual void OnMoveCompleted(bool bSuccess);

    // ========================================================================
    // 내부 이동 처리
    // ========================================================================
    void UpdateMovement(float DeltaTime);
    void UpdateRotation(float DeltaTime);

protected:
    // ========== AI 상태 ==========
    UPROPERTY(EditAnywhere, Category = "AI")
    bool bAIEnabled = true;

    // ========== 타겟 ==========
    AActor* TargetActor = nullptr;

    // ========== 이동 ==========
    UPROPERTY(EditAnywhere, Category = "Movement")
    float MoveSpeed = 300.f;

    UPROPERTY(EditAnywhere, Category = "Movement")
    float RotationSpeed = 5.f;

    UPROPERTY(EditAnywhere, Category = "Movement")
    float AcceptanceRadius = 100.f;

    FVector MoveDestination = FVector();
    bool bIsMoving = false;
    bool bHasMoveGoal = false;

    // ========== 회전 ==========
    FVector FocusLocation = FVector();
    bool bHasFocusPoint = false;
};
