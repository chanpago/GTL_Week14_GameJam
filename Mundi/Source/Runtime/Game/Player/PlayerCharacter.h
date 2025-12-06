#pragma once

#include "Character.h"
#include "IDamageable.h"
#include "CombatTypes.h"
#include "APlayerCharacter.generated.h"
class UStatsComponent;
class UHitboxComponent;
class USpringArmComponent;
class UCameraComponent;
class UAnimMontage;

// ============================================================================
// APlayerCharacter - 플레이어 캐릭터 예시
// ============================================================================

UCLASS(DisplayName = "플레이어 캐릭터", Description = "플레이어가 조작하는 캐릭터")
class APlayerCharacter : public ACharacter, public IDamageable
{
public:
    GENERATED_REFLECTION_BODY()

    APlayerCharacter();
    virtual ~APlayerCharacter() = default;

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
    virtual bool IsBlocking() const override;
    virtual bool IsParrying() const override;
    virtual void OnHitReaction(EHitReaction Reaction, const FDamageInfo& DamageInfo) override;
    virtual void OnDeath() override;

    // ========================================================================
    // 전투 액션
    // ========================================================================

    /** 약공격 */
    void LightAttack();

    /** 강공격 */
    void HeavyAttack();

    /** 회피 (구르기) */
    void Dodge();

    /** 가드 시작 */
    void StartBlock();

    /** 가드 종료 */
    void StopBlock();

    // ========================================================================
    // 상태 확인
    // ========================================================================
    ECombatState GetCombatState() const { return CombatState; }
    bool IsInvincible() const { return bIsInvincible; }

    // ========================================================================
    // 컴포넌트 접근
    // ========================================================================
    UStatsComponent* GetStatsComponent() const { return StatsComponent; }

protected:
    // ========================================================================
    // 입력 처리
    // ========================================================================
    void ProcessInput(float DeltaTime);
    void ProcessMovementInput(float DeltaTime);
    void ProcessCombatInput();

    // ========================================================================
    // 콜백
    // ========================================================================
    void HandleHealthChanged(float CurrentHealth, float MaxHealth);
    void HandleStaminaChanged(float CurrentStamina, float MaxStamina);
    void HandleDeath();

    // ========================================================================
    // 내부 함수
    // ========================================================================
    void SetCombatState(ECombatState NewState);
    void UpdateAttackState(float DeltaTime);
    void UpdateParryWindow(float DeltaTime);
    void UpdateStagger(float DeltaTime);
    void UpdateDodgeState(float DeltaTime);

    /** 입력 방향을 기반으로 8방향 인덱스 반환 (0=F, 1=FR, 2=R, 3=BR, 4=B, 5=BL, 6=L, 7=FL) */
    int32 GetDodgeDirectionIndex() const;

protected:
    // ========== 컴포넌트 ==========
    UStatsComponent* StatsComponent = nullptr;
    UHitboxComponent* HitboxComponent = nullptr;
    USpringArmComponent* SpringArm = nullptr;
    UCameraComponent* Camera = nullptr;

    // ========== 전투 상태 ==========
    ECombatState CombatState = ECombatState::Idle;

    // ========== 상태 플래그 ==========
    UPROPERTY(EditAnywhere, Category = "Combat")
    bool bIsInvincible = false;         // 무적 (회피 중)

    UPROPERTY(EditAnywhere, Category = "Combat")
    bool bIsBlocking = false;           // 가드 중

    UPROPERTY(EditAnywhere, Category = "Combat")
    bool bIsParrying = false;           // 패리 윈도우

    bool bCanCombo = false;             // 콤보 입력 가능

    // ========== 타이머 ==========
    float ParryWindowTimer = 0.f;

    UPROPERTY(EditAnywhere, Category = "Combat", Tooltip = "패리 판정 시간")
    float ParryWindowDuration = 0.2f;

    float StaggerTimer = 0.f;

    // ========== 콤보 ==========
    UPROPERTY(EditAnywhere, Category = "Combat")
    int32 ComboCount = 0;

    UPROPERTY(EditAnywhere, Category = "Combat")
    int32 MaxComboCount = 3;

    // ========== 몽타주 ==========
    /** 공격 애니메이션 경로 */
    UPROPERTY(EditAnywhere, Category = "Animation")
    FString LightAttackAnimPath;

    /** 공격 몽타주 */
    UAnimMontage* LightAttackMontage = nullptr;

    // ========== 구르기 몽타주 (8방향) ==========
    /** 구르기 애니메이션 경로 - Forward */
    UPROPERTY(EditAnywhere, Category = "Animation|Dodge")
    FString DodgeAnimPath_F;

    /** 구르기 애니메이션 경로 - Forward-Right */
    UPROPERTY(EditAnywhere, Category = "Animation|Dodge")
    FString DodgeAnimPath_FR;

    /** 구르기 애니메이션 경로 - Right */
    UPROPERTY(EditAnywhere, Category = "Animation|Dodge")
    FString DodgeAnimPath_R;

    /** 구르기 애니메이션 경로 - Backward-Right */
    UPROPERTY(EditAnywhere, Category = "Animation|Dodge")
    FString DodgeAnimPath_BR;

    /** 구르기 애니메이션 경로 - Backward */
    UPROPERTY(EditAnywhere, Category = "Animation|Dodge")
    FString DodgeAnimPath_B;

    /** 구르기 애니메이션 경로 - Backward-Left */
    UPROPERTY(EditAnywhere, Category = "Animation|Dodge")
    FString DodgeAnimPath_BL;

    /** 구르기 애니메이션 경로 - Left */
    UPROPERTY(EditAnywhere, Category = "Animation|Dodge")
    FString DodgeAnimPath_L;

    /** 구르기 애니메이션 경로 - Forward-Left */
    UPROPERTY(EditAnywhere, Category = "Animation|Dodge")
    FString DodgeAnimPath_FL;

    /** 구르기 몽타주 배열 (8방향) - 인덱스: 0=F, 1=FR, 2=R, 3=BR, 4=B, 5=BL, 6=L, 7=FL */
    UAnimMontage* DodgeMontages[8] = { nullptr };

    /** 구르기 시 루트 모션 활성화 여부 */
    UPROPERTY(EditAnywhere, Category = "Animation|Dodge")
    bool bEnableDodgeRootMotion = true;

    /** 구르기 애니메이션 끝에서 자를 시간 (초 단위) */
    UPROPERTY(EditAnywhere, Category = "Animation|Dodge")
    float DodgeAnimationCutEndTime = 0.0f;

    /** 공격 시 루트 모션 활성화 여부 */
    UPROPERTY(EditAnywhere, Category = "Animation")
    bool bEnableAttackRootMotion = true;

    /** 애니메이션 끝에서 자를 시간 (초 단위) - 제자리 복귀 애니메이션 자르기용 */
    UPROPERTY(EditAnywhere, Category = "Animation")
    float AnimationCutEndTime = 0.0f;

    // ========== 이동 ==========
    UPROPERTY(EditAnywhere, Category = "Movement")
    float MoveSpeed = 500.f;

    UPROPERTY(EditAnywhere, Category = "Movement")
    float RotationSpeed = 10.f;
};
