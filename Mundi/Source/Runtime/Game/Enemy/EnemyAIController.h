#pragma once

#include "AIController.h"
#include "CombatTypes.h"
#include "AEnemyAIController.generated.h"

class AEnemyBase;

// ============================================================================
// AI 행동 상태
// ============================================================================
enum class EAIBehaviorState
{
    Idle,           // 대기
    Patrol,         // 순찰
    Chase,          // 추적
    Attack,         // 공격
    Strafe,         // 좌우 이동 (전투 중)
    Retreat,        // 후퇴
    Stagger,        // 경직
    Dead            // 사망
};

// ============================================================================
// AEnemyAIController - 적 AI 컨트롤러
// ============================================================================

UCLASS(DisplayName = "적 AI 컨트롤러", Description = "적 캐릭터 AI 컨트롤러")
class AEnemyAIController : public AAIController
{
public:
    GENERATED_REFLECTION_BODY()

    AEnemyAIController();
    virtual ~AEnemyAIController() override = default;

    // ========================================================================
    // Lifecycle
    // ========================================================================
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    // ========================================================================
    // 빙의
    // ========================================================================
    virtual void Possess(APawn* InPawn) override;

    // ========================================================================
    // AI 상태
    // ========================================================================
    EAIBehaviorState GetBehaviorState() const { return BehaviorState; }
    void SetBehaviorState(EAIBehaviorState NewState);

    // ========================================================================
    // 전투
    // ========================================================================

    /** 공격 실행 요청 */
    void RequestAttack();

    /** 경직 상태 진입 */
    void EnterStagger(float Duration);

    /** 사망 처리 */
    void OnPawnDeath();

protected:
    // ========================================================================
    // AI 로직 (오버라이드)
    // ========================================================================
    virtual void UpdateAI(float DeltaTime) override;
    virtual void OnMoveCompleted(bool bSuccess) override;

    // ========================================================================
    // 상태별 업데이트
    // ========================================================================
    virtual void UpdateIdle(float DeltaTime);
    virtual void UpdatePatrol(float DeltaTime);
    virtual void UpdateChase(float DeltaTime);
    virtual void UpdateAttack(float DeltaTime);
    virtual void UpdateStrafe(float DeltaTime);
    virtual void UpdateRetreat(float DeltaTime);
    virtual void UpdateStagger(float DeltaTime);

    // ========================================================================
    // 감지
    // ========================================================================

    /** 플레이어 감지 시도 */
    virtual bool TryDetectPlayer();

    /** 현재 타겟이 유효한지 확인 */
    virtual bool IsTargetValid() const;

    /** 타겟이 공격 범위 내인지 확인 */
    virtual bool IsTargetInAttackRange() const;

    /** 타겟을 잃었는지 확인 */
    virtual bool HasLostTarget() const;

    // ========================================================================
    // 공격
    // ========================================================================

    /** 공격 패턴 선택 */
    virtual int32 SelectAttackPattern();

    /** 공격 실행 */
    virtual void ExecuteAttack(int32 PatternIndex);

public:
    /** 공격 완료 처리 (EnemyBase에서 호출) */
    virtual void OnAttackFinished();

protected:

    // ========================================================================
    // 유틸리티
    // ========================================================================
    AEnemyBase* GetEnemyPawn() const;

protected:
    // ========== AI 상태 ==========
    EAIBehaviorState BehaviorState = EAIBehaviorState::Idle;
    EAIBehaviorState PreviousState = EAIBehaviorState::Idle;

    // ========== 감지 설정 ==========
    UPROPERTY(EditAnywhere, Category = "AI|Detection", Tooltip = "플레이어 감지 거리")
    float DetectionRange = 1000.f;

    UPROPERTY(EditAnywhere, Category = "AI|Detection", Tooltip = "공격 사거리")
    float AttackRange = 200.f;

    UPROPERTY(EditAnywhere, Category = "AI|Detection", Tooltip = "타겟 잃는 거리")
    float LoseTargetRange = 1500.f;

    // ========== 공격 ==========
    UPROPERTY(EditAnywhere, Category = "AI|Combat", Tooltip = "공격 쿨타임")
    float AttackCooldown = 2.0f;

    float AttackCooldownTimer = 0.f;

    UPROPERTY(EditAnywhere, Category = "AI|Combat")
    int32 MaxAttackPatterns = 2;

    int32 LastAttackPattern = -1;

    // ========== 경직 ==========
    float StaggerTimer = 0.f;

    // ========== 전투 행동 ==========
    UPROPERTY(EditAnywhere, Category = "AI|Combat", Tooltip = "전투 중 좌우 이동 확률")
    float StrafeChance = 0.3f;

    UPROPERTY(EditAnywhere, Category = "AI|Combat", Tooltip = "좌우 이동 시간")
    float StrafeDuration = 1.5f;

    float StrafeTimer = 0.f;
    float StrafeDirection = 1.f; // 1 = 오른쪽, -1 = 왼쪽

    // ========== 상태 플래그 ==========
    bool bIsAttacking = false;
    bool bAttackRequested = false;
};
