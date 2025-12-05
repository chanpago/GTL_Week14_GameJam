#pragma once

#include "EnemyBase.h"
#include "ABossEnemy.generated.h"

// ============================================================================
// ABossEnemy - 보스 적 클래스
// ============================================================================

UCLASS(DisplayName = "보스 에너미", Description = "보스 적 캐릭터")
class ABossEnemy : public AEnemyBase
{
public:
    GENERATED_REFLECTION_BODY()

    ABossEnemy();
    virtual ~ABossEnemy() = default;

    // ========================================================================
    // Lifecycle
    // ========================================================================
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    // ========================================================================
    // 보스 전용 공격 패턴
    // ========================================================================
    virtual void ExecuteAttackPattern(int32 PatternIndex) override;

    /** 페이즈 변경 */
    void SetPhase(int32 NewPhase);
    int32 GetPhase() const { return CurrentPhase; }

protected:
    // ========================================================================
    // 보스 공격 패턴들
    // ========================================================================
    void Attack_LightCombo();       // 약공격 콤보
    void Attack_HeavySlam();        // 강공격 내려찍기
    void Attack_ChargeAttack();     // 돌진 공격
    void Attack_SpinAttack();       // 회전 공격

    // ========================================================================
    // 페이즈 전환
    // ========================================================================
    void CheckPhaseTransition();
    void OnPhaseChanged(int32 OldPhase, int32 NewPhase);

protected:
    // ========== 보스 설정 ==========
    UPROPERTY(EditAnywhere, Category = "Boss")
    int32 CurrentPhase = 1;

    UPROPERTY(EditAnywhere, Category = "Boss")
    int32 MaxPhase = 2;

    UPROPERTY(EditAnywhere, Category = "Boss", Tooltip = "페이즈 2 진입 HP 비율")
    float Phase2HealthThreshold = 0.5f;
};
