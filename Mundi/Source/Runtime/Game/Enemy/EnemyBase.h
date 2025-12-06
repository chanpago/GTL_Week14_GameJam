#pragma once

#include "Character.h"
#include "IDamageable.h"
#include "CombatTypes.h"
#include "AenemyBase.generated.h"

class UStatsComponent;
class UHitboxComponent;
class AEnemyAIController;

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

    /** AIController 접근 */
    AEnemyAIController* GetAIController() const { return AIController; }

    /** 슈퍼아머 상태 */
    bool HasSuperArmor() const { return bHasSuperArmor; }
    void SetSuperArmor(bool bEnabled) { bHasSuperArmor = bEnabled; }

    // ========================================================================
    // 컴포넌트 접근
    // ========================================================================
    UStatsComponent* GetStatsComponent() const { return StatsComponent; }
    UHitboxComponent* GetHitboxComponent() const { return HitboxComponent; }

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
    // 공격 (AIController에서 호출)
    // ========================================================================
    virtual void StartAttack();

public:
    /** AIController에서 호출하는 공격 실행 */
  
    virtual void ExecuteAttackPattern(int PatternIndex);

    /** 공격 완료 알림 (애니메이션 노티파이에서 호출) */
    virtual void NotifyAttackFinished();

protected:

    // ========================================================================
    // 콜백
    // ========================================================================
    void HandleDeath();


    // ========== 컴포넌트 ==========
    UStatsComponent* StatsComponent = nullptr;
    UHitboxComponent* HitboxComponent = nullptr;

    // ========== AI 컨트롤러 ==========
    AEnemyAIController* AIController = nullptr;

    // ========== AI 상태 ==========
    EEnemyAIState AIState = EEnemyAIState::Idle;
    AActor* TargetActor = nullptr;
public:
    // ========== 감지 설정 ==========
    UPROPERTY(EditAnywhere, Category = "AI", Tooltip = "플레이어 감지 거리")
    float DetectionRange = 1000.f;

    UPROPERTY(EditAnywhere, Category = "AI", Tooltip = "공격 사거리")
    float AttackRange = 200.f;

    UPROPERTY(EditAnywhere, Category = "AI", Tooltip = "타겟 잃는 거리")
    float LoseTargetRange = 1500.f;

    // ========== 이동 ==========
    UPROPERTY(EditAnywhere, Category = "Movement")
    float MoveSpeed = 300.f;

    UPROPERTY(EditAnywhere, Category = "Movement")
    float RotationSpeed = 5.f;

    // ========== 공격 ==========
    UPROPERTY(EditAnywhere, Category = "Combat", Tooltip = "공격 쿨타임")
    float AttackCooldown = 2.0f;

    float AttackTimer = 0.f;

    UPROPERTY(EditAnywhere, Category = "Combat")
    int32 CurrentAttackPattern = 0;

    UPROPERTY(EditAnywhere, Category = "Combat")
    int32 MaxAttackPatterns = 2;

    // ========== 경직 ==========
    float StaggerTimer = 0.f;

    // ========== 상태 플래그 ==========
    UPROPERTY(EditAnywhere, Category = "Combat")
    bool bIsAttacking = false;

    UPROPERTY(EditAnywhere, Category = "Combat", Tooltip = "슈퍼아머 활성화")
    bool bHasSuperArmor = false;

public:
    // ========== 프리팹 설정 (에디터에서 지정) ==========

    /** 적 메시 프리팹 경로 */
    UPROPERTY(EditAnywhere, Category = "Prefab", DisplayName = "메시 프리팹 경로")
    FString MeshPrefabPath;

    /** 프리팹에서 메시 로드 */
    void LoadMeshFromPrefab();

    /** 프리팹 경로로 적 스폰 (static) */
    static AEnemyBase* SpawnFromPrefab(UWorld* World, const FString& PrefabPath, const FVector& Location);
};
