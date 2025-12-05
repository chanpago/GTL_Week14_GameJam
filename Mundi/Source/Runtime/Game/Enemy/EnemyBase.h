#pragma once

#include "Character.h"
#include "IDamageable.h"
#include "CombatTypes.h"
#include "AenemyBase.generated.h"

class UStatsComponent;
class UHitboxComponent;

// ============================================================================
// AI 상태
// ============================================================================
enum class EEnemyAIState
{
    Idle,       // 대기
    Patrol,     // 순찰
    Chase,      // 추적
    Attack,     // 공격
    Stagger,    // 경직
    Dead        // 사망
};

// ============================================================================
// AEnemyBase - 적 캐릭터 베이스 클래스
// ============================================================================
UCLASS(DisplayName = "AEnemyBase", Description = "적 기본 객체")
class AEnemyBase : public ACharacter, public IDamageable
{
public:
    GENERATED_REFLECTION_BODY()

    AEnemyBase();
    virtual ~AEnemyBase() = default;

    // ========================================================================
    // Lifecycle
    // ========================================================================
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    // ========================================================================
    // IDamageable 구현
    // ========================================================================
    virtual float TakeDamage(const FDamageInfo& DamageInfo) override;
    virtual bool IsAlive() const override;
    virtual bool CanBeHit() const override;
    virtual bool IsBlocking() const override { return false; }
    virtual bool IsParrying() const override { return false; }
    virtual void OnHitReaction(EHitReaction Reaction, const FDamageInfo& DamageInfo) override;
    virtual void OnDeath() override;

    // ========================================================================
    // AI
    // ========================================================================
    EEnemyAIState GetAIState() const { return AIState; }
    void SetTarget(AActor* NewTarget) { TargetActor = NewTarget; }
    AActor* GetTarget() const { return TargetActor; }

    // ========================================================================
    // 컴포넌트 접근
    // ========================================================================
    UStatsComponent* GetStatsComponent() const { return StatsComponent; }

protected:
    // ========================================================================
    // AI 로직
    // ========================================================================
    virtual void UpdateAI(float DeltaTime);
    virtual void SetAIState(EEnemyAIState NewState);

    // 상태별 업데이트
    virtual void UpdateIdle(float DeltaTime);
    virtual void UpdateChase(float DeltaTime);
    virtual void UpdateAttack(float DeltaTime);
    virtual void UpdateStagger(float DeltaTime);

    // ========================================================================
    // 감지
    // ========================================================================
    virtual bool DetectPlayer();
    virtual bool IsPlayerInAttackRange();
    virtual void LookAtTarget(float DeltaTime);
    virtual void MoveToTarget(float DeltaTime);

    // ========================================================================
    // 공격
    // ========================================================================
    virtual void StartAttack();
    virtual void ExecuteAttackPattern(int32 PatternIndex);

    // ========================================================================
    // 콜백
    // ========================================================================
    void HandleDeath();

protected:
    // ========== 컴포넌트 ==========
    UStatsComponent* StatsComponent = nullptr;
    UHitboxComponent* HitboxComponent = nullptr;

    // ========== AI 상태 ==========
    EEnemyAIState AIState = EEnemyAIState::Idle;
    AActor* TargetActor = nullptr;

    // ========== 감지 설정 ==========
    float DetectionRange = 1000.f;      // 플레이어 감지 거리
    float AttackRange = 200.f;          // 공격 사거리
    float LoseTargetRange = 1500.f;     // 타겟 잃는 거리

    // ========== 이동 ==========
    float MoveSpeed = 300.f;
    float RotationSpeed = 5.f;

    // ========== 공격 ==========
    float AttackCooldown = 2.0f;        // 공격 쿨타임
    float AttackTimer = 0.f;
    int32 CurrentAttackPattern = 0;
    int32 MaxAttackPatterns = 2;

    // ========== 경직 ==========
    float StaggerTimer = 0.f;

    // ========== 상태 플래그 ==========
    bool bIsAttacking = false;
    bool bHasSuperArmor = false;
};
