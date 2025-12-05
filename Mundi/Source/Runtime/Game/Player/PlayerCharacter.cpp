#include "pch.h"
#include "PlayerCharacter.h"
#include "StatsComponent.h"
#include "HitboxComponent.h"
#include "SpringArmComponent.h"
#include "CameraComponent.h"
#include "InputManager.h"
#include "SkeletalMeshComponent.h"
#include "World.h"
#include "GameModeBase.h"
#include "GameState.h"



APlayerCharacter::APlayerCharacter()
{
    // 스탯 컴포넌트
    StatsComponent = CreateDefaultSubobject<UStatsComponent>("StatsComponent");

    // 히트박스 컴포넌트 (무기에 붙일 수도 있음)
    HitboxComponent = CreateDefaultSubobject<UHitboxComponent>("HitboxComponent");
    HitboxComponent->SetBoxExtent(FVector(50.f, 50.f, 50.f));

    // 스프링암 + 카메라 (필요시)
    // SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
    // Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
}

void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    // 델리게이트 바인딩
    if (StatsComponent)
    {
        // StatsComponent->OnHealthChanged.AddDynamic(this, &APlayerCharacter::HandleHealthChanged);
        // StatsComponent->OnStaminaChanged.AddDynamic(this, &APlayerCharacter::HandleStaminaChanged);
        // StatsComponent->OnDeath.AddDynamic(this, &APlayerCharacter::HandleDeath);
    }

    // 히트박스 소유자 설정
    if (HitboxComponent)
    {
        HitboxComponent->SetOwnerActor(this);
    }
}

void APlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // Only process gameplay when in Fighting state
    if (AGameModeBase* GM = GWorld ? GWorld->GetGameMode() : nullptr)
    {
        if (AGameState* GS = Cast<AGameState>(GM->GetGameState()))
        {
            if (GS->GetGameFlowState() != EGameFlowState::Fighting)
            {
                return;
            }
        }
    }

    // 사망 상태면 입력 무시
    if (CombatState == ECombatState::Dead)
    {
        return;
    }

    // 경직 상태 업데이트
    UpdateStagger(DeltaSeconds);

    // 패리 윈도우 업데이트
    UpdateParryWindow(DeltaSeconds);

    // 경직 중이 아니면 입력 처리
    if (CombatState != ECombatState::Staggered)
    {
        ProcessInput(DeltaSeconds);
    }
}

// ============================================================================
// 입력 처리
// ============================================================================

void APlayerCharacter::ProcessInput(float DeltaTime)
{
    //ProcessMovementInput(DeltaTime);
    ProcessCombatInput();
}

void APlayerCharacter::ProcessMovementInput(float DeltaTime)
{
    // 공격/회피 중에는 이동 불가
    if (CombatState == ECombatState::Attacking || CombatState == ECombatState::Dodging)
    {
        return;
    }

    FVector MoveDirection = FVector();

    if (INPUT.IsKeyDown('W')) MoveDirection.Y += 1.f;
    if (INPUT.IsKeyDown('S')) MoveDirection.Y -= 1.f;
    if (INPUT.IsKeyDown('A')) MoveDirection.X -= 1.f;
    if (INPUT.IsKeyDown('D')) MoveDirection.X += 1.f;

    if (MoveDirection.SizeSquared() > 0.f)
    {
        MoveDirection.Normalize();
        FVector NewLocation = GetActorLocation() + MoveDirection * MoveSpeed * DeltaTime;
        SetActorLocation(NewLocation);

        // 이동 방향으로 회전
        // FQuat TargetRotation = FQuat::LookAt(MoveDirection, FVector::UpVector);
        // SetActorRotation(TargetRotation);
    }
}

void APlayerCharacter::ProcessCombatInput()
{
    // 마우스 좌클릭 - 약공격
    if (INPUT.IsMouseButtonPressed(LeftButton))
    {
        LightAttack();
    }

    // 마우스 우클릭 - 가드
    if (INPUT.IsMouseButtonPressed(RightButton))
    {
        StartBlock();
    }
    if (INPUT.IsMouseButtonReleased(RightButton))
    {
        StopBlock();
    }

    // 스페이스 - 회피
    if (INPUT.IsKeyPressed(VK_SPACE))
    {
        Dodge();
    }

    // Shift + 좌클릭 - 강공격
    if (INPUT.IsKeyDown(VK_SHIFT) && INPUT.IsMouseButtonPressed(LeftButton))
    {
        HeavyAttack();
    }
}

// ============================================================================
// 전투 액션
// ============================================================================

void APlayerCharacter::LightAttack()
{
    // 상태 체크
    if (CombatState == ECombatState::Attacking && !bCanCombo)
    {
        return;
    }
    if (CombatState == ECombatState::Staggered || CombatState == ECombatState::Dead)
    {
        return;
    }

    // 스태미나 체크
    if (!StatsComponent->ConsumeStamina(StatsComponent->LightAttackCost))
    {
        return; // 스태미나 부족
    }

    // 가드 해제
    StopBlock();

    SetCombatState(ECombatState::Attacking);

    // 콤보 카운트 증가
    if (bCanCombo)
    {
        ComboCount = (ComboCount + 1) % MaxComboCount;
    }
    else
    {
        ComboCount = 0;
    }

    // 히트박스 활성화
    FDamageInfo DamageInfo(this, 10.f + ComboCount * 5.f, EDamageType::Light);
    HitboxComponent->EnableHitbox(DamageInfo);

    // TODO: 공격 애니메이션 재생
    // PlayAttackAnimation(ComboCount);

    bCanCombo = false;
}

void APlayerCharacter::HeavyAttack()
{
    if (CombatState == ECombatState::Attacking ||
        CombatState == ECombatState::Staggered ||
        CombatState == ECombatState::Dead)
    {
        return;
    }

    if (!StatsComponent->ConsumeStamina(StatsComponent->HeavyAttackCost))
    {
        return;
    }

    StopBlock();
    SetCombatState(ECombatState::Attacking);
    ComboCount = 0;

    FDamageInfo DamageInfo(this, 30.f, EDamageType::Heavy);
    DamageInfo.HitReaction = EHitReaction::Stagger;
    DamageInfo.StaggerDuration = 0.5f;
    HitboxComponent->EnableHitbox(DamageInfo);

    // TODO: 강공격 애니메이션 재생
}

void APlayerCharacter::Dodge()
{
    if (CombatState == ECombatState::Dodging ||
        CombatState == ECombatState::Staggered ||
        CombatState == ECombatState::Dead)
    {
        return;
    }

    if (!StatsComponent->ConsumeStamina(StatsComponent->DodgeCost))
    {
        return;
    }

    StopBlock();
    SetCombatState(ECombatState::Dodging);
    bIsInvincible = true;

    // TODO: 구르기 애니메이션 재생
    // 애니메이션 노티파이에서 bIsInvincible = false 처리
}

void APlayerCharacter::StartBlock()
{
    if (CombatState == ECombatState::Attacking ||
        CombatState == ECombatState::Dodging ||
        CombatState == ECombatState::Dead)
    {
        return;
    }

    bIsBlocking = true;
    SetCombatState(ECombatState::Blocking);

    // 패리 윈도우 시작
    bIsParrying = true;
    ParryWindowTimer = ParryWindowDuration;

    // TODO: 가드 애니메이션 재생
}

void APlayerCharacter::StopBlock()
{
    if (!bIsBlocking)
    {
        return;
    }

    bIsBlocking = false;
    bIsParrying = false;
    ParryWindowTimer = 0.f;

    if (CombatState == ECombatState::Blocking)
    {
        SetCombatState(ECombatState::Idle);
    }
}

// ============================================================================
// IDamageable 구현
// ============================================================================

float APlayerCharacter::TakeDamage(const FDamageInfo& DamageInfo)
{
    if (!CanBeHit())
    {
        return 0.f;
    }

    float ActualDamage = DamageInfo.Damage;

    // 가드 중이면 데미지 감소
    if (bIsBlocking && DamageInfo.bCanBeBlocked)
    {
        ActualDamage *= 0.2f; // 80% 감소
        StatsComponent->ConsumeStamina(StatsComponent->BlockCostPerHit);
    }

    StatsComponent->ApplyDamage(ActualDamage);

    // 피격 반응 (가드 중이 아니거나 슈퍼아머가 아니면)
    if (!bIsBlocking)
    {
        OnHitReaction(DamageInfo.HitReaction, DamageInfo);
    }

    return ActualDamage;
}

bool APlayerCharacter::IsAlive() const
{
    return StatsComponent && StatsComponent->IsAlive();
}

bool APlayerCharacter::CanBeHit() const
{
    return IsAlive() && !bIsInvincible;
}

bool APlayerCharacter::IsBlocking() const
{
    return bIsBlocking;
}

bool APlayerCharacter::IsParrying() const
{
    return bIsParrying;
}

void APlayerCharacter::OnHitReaction(EHitReaction Reaction, const FDamageInfo& DamageInfo)
{
    if (Reaction == EHitReaction::None)
    {
        return;
    }

    // 현재 공격/회피 중단
    HitboxComponent->DisableHitbox();
    bIsInvincible = false;

    SetCombatState(ECombatState::Staggered);
    StaggerTimer = DamageInfo.StaggerDuration;

    // TODO: 피격 애니메이션 재생
    // PlayHitReactionAnimation(Reaction);

    // 넉백 적용
    if (Reaction == EHitReaction::Knockback && DamageInfo.KnockbackForce > 0.f)
    {
        FVector KnockbackDir = DamageInfo.HitDirection * DamageInfo.KnockbackForce;
        AddActorWorldLocation(KnockbackDir * 0.1f); // 간단한 넉백
    }
}

void APlayerCharacter::OnDeath()
{
    SetCombatState(ECombatState::Dead);
    HitboxComponent->DisableHitbox();

    // TODO: 사망 애니메이션/래그돌
}

// ============================================================================
// 내부 함수
// ============================================================================

void APlayerCharacter::SetCombatState(ECombatState NewState)
{
    ECombatState OldState = CombatState;
    CombatState = NewState;

    // 상태 변경 시 처리
    if (OldState == ECombatState::Attacking && NewState != ECombatState::Attacking)
    {
        HitboxComponent->DisableHitbox();
        HitboxComponent->ClearHitActors();
    }
}

void APlayerCharacter::UpdateParryWindow(float DeltaTime)
{
    if (bIsParrying && ParryWindowTimer > 0.f)
    {
        ParryWindowTimer -= DeltaTime;
        if (ParryWindowTimer <= 0.f)
        {
            bIsParrying = false;
        }
    }
}

void APlayerCharacter::UpdateStagger(float DeltaTime)
{
    if (CombatState == ECombatState::Staggered && StaggerTimer > 0.f)
    {
        StaggerTimer -= DeltaTime;
        if (StaggerTimer <= 0.f)
        {
            SetCombatState(ECombatState::Idle);
        }
    }
}

// ============================================================================
// 콜백
// ============================================================================

void APlayerCharacter::HandleHealthChanged(float CurrentHealth, float MaxHealth)
{
    // UI 업데이트는 UI 클래스에서 델리게이트 바인딩해서 처리
}

void APlayerCharacter::HandleStaminaChanged(float CurrentStamina, float MaxStamina)
{
    // UI 업데이트는 UI 클래스에서 델리게이트 바인딩해서 처리
}

void APlayerCharacter::HandleDeath()
{
    OnDeath();
}
