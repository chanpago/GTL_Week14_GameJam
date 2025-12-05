#include "pch.h"
#include "StatsComponent.h"


UStatsComponent::UStatsComponent()
{
    bCanEverTick = true;
    bTickEnabled = true;
}

void UStatsComponent::BeginPlay()
{
    Super::BeginPlay();

    // 시작 시 풀 HP/스태미나
    CurrentHealth = MaxHealth;
    CurrentStamina = MaxStamina;

    // 초기 상태 브로드캐스트
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
    OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}

void UStatsComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    // 스태미나 자동 회복
    if (bCanRegenStamina && CurrentStamina < MaxStamina)
    {
        TimeSinceStaminaUse += DeltaTime;

        if (TimeSinceStaminaUse >= StaminaRegenDelay)
        {
            float OldStamina = CurrentStamina;
            CurrentStamina = FMath::Min(MaxStamina, CurrentStamina + StaminaRegenRate * DeltaTime);

            // 값이 변경되었을 때만 브로드캐스트
            if (CurrentStamina != OldStamina)
            {
                OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
            }
        }
    }
}

// ============================================================================
// HP 관련
// ============================================================================

void UStatsComponent::ApplyDamage(float Damage)
{
    if (Damage <= 0.f || !IsAlive())
    {
        return;
    }

    float OldHealth = CurrentHealth;
    CurrentHealth = FMath::Max(0.f, CurrentHealth - Damage);

    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

    // 사망 체크
    if (OldHealth > 0.f && CurrentHealth <= 0.f)
    {
        OnDeath.Broadcast();
    }
}

void UStatsComponent::Heal(float Amount)
{
    if (Amount <= 0.f || !IsAlive())
    {
        return;
    }

    CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + Amount);
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UStatsComponent::SetHealth(float NewHealth)
{
    CurrentHealth = FMath::Clamp(NewHealth, 0.f, MaxHealth);
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

    if (CurrentHealth <= 0.f)
    {
        OnDeath.Broadcast();
    }
}

void UStatsComponent::RestoreFullHealth()
{
    CurrentHealth = MaxHealth;
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

// ============================================================================
// 스태미나 관련
// ============================================================================

bool UStatsComponent::ConsumeStamina(float Amount)
{
    if (Amount <= 0.f)
    {
        return true;
    }

    if (CurrentStamina < Amount)
    {
        return false; // 스태미나 부족
    }

    CurrentStamina -= Amount;
    TimeSinceStaminaUse = 0.f; // 회복 딜레이 리셋

    OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
    return true;
}

void UStatsComponent::RecoverStamina(float Amount)
{
    if (Amount <= 0.f)
    {
        return;
    }

    CurrentStamina = FMath::Min(MaxStamina, CurrentStamina + Amount);
    OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}

void UStatsComponent::RestoreFullStamina()
{
    CurrentStamina = MaxStamina;
    TimeSinceStaminaUse = StaminaRegenDelay; // 즉시 회복 가능 상태로
    OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}
