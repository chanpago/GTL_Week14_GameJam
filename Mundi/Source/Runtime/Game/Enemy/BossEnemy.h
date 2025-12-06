#pragma once

#include "EnemyBase.h"
#include "ABossEnemy.generated.h"

class UAnimMontage;

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
    UFUNCTION(LuaBind,DisplayName='Attack',Tooltip="Play AttackPattern")
    void ExecuteAttackPattern(int PatternIndex);

    /** 몽타주 이름으로 재생 (Lua용) */
    bool PlayMontageByName(const FString& MontageName, float BlendIn = 0.1f, float BlendOut = 0.1f, float PlayRate = 1.0f);

    /** 현재 재생 중인 몽타주의 재생 속도 변경 (Lua용) */
    void SetMontagePlayRate(float NewPlayRate);

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

    // ========== 공격 애니메이션 경로 ==========
    UPROPERTY(EditAnywhere, Category = "Animation")
    FString LightComboAnimPath;

    UPROPERTY(EditAnywhere, Category = "Animation")
    FString HeavySlamAnimPath;

    UPROPERTY(EditAnywhere, Category = "Animation")
    FString ChargeStartAnimPath;

    UPROPERTY(EditAnywhere, Category = "Animation")
    FString ChargeAttackAnimPath;

    UPROPERTY(EditAnywhere, Category = "Animation")
    FString SpinAttackAnimPath;

    // ========== 공격 애니메이션 몽타주 ==========
    UAnimMontage* LightComboMontage = nullptr;
    UAnimMontage* HeavySlamMontage = nullptr;
    UAnimMontage* ChargeStartMontage = nullptr;
    UAnimMontage* ChargeAttackMontage = nullptr;
    UAnimMontage* SpinAttackMontage = nullptr;

    // ========== 애니메이션 설정 ==========
    UPROPERTY(EditAnywhere, Category = "Animation")
    bool bEnableAttackRootMotion = true;

    UPROPERTY(EditAnywhere, Category = "Animation")
    float AnimationCutEndTime = 0.0f;
};
