#pragma once

#include "Character.h"
#include "IDamageable.h"
#include "CombatTypes.h"
#include "APlayerCharacter.generated.h"
class UStatsComponent;
class UHitboxComponent;
class USpringArmComponent;
class UCameraComponent;

// ============================================================================
// APlayerCharacter - 플레이어 캐릭터 예시
// ============================================================================

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
    void UpdateParryWindow(float DeltaTime);
    void UpdateStagger(float DeltaTime);

protected:
    // ========== 컴포넌트 ==========
    UStatsComponent* StatsComponent = nullptr;
    UHitboxComponent* HitboxComponent = nullptr;
    USpringArmComponent* SpringArm = nullptr;
    UCameraComponent* Camera = nullptr;

    // ========== 전투 상태 ==========
    ECombatState CombatState = ECombatState::Idle;

    // ========== 상태 플래그 ==========
    bool bIsInvincible = false;         // 무적 (회피 중)
    bool bIsBlocking = false;           // 가드 중
    bool bIsParrying = false;           // 패리 윈도우
    bool bCanCombo = false;             // 콤보 입력 가능

    // ========== 타이머 ==========
    float ParryWindowTimer = 0.f;
    float ParryWindowDuration = 0.2f;   // 패리 판정 시간
    float StaggerTimer = 0.f;

    // ========== 콤보 ==========
    int32 ComboCount = 0;
    int32 MaxComboCount = 3;

    // ========== 이동 ==========
    float MoveSpeed = 500.f;
    float RotationSpeed = 10.f;
};
