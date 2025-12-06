#include "pch.h"
#include "BossEnemy.h"
#include "StatsComponent.h"
#include "HitboxComponent.h"
#include "World.h"
#include "SkeletalMeshComponent.h"
#include "AnimInstance.h"
#include "AnimMontage.h"
#include "AnimSequence.h"
#include "ResourceManager.h"

ABossEnemy::ABossEnemy()
{
    // 보스 기본 스탯 설정
    if (StatsComponent)
    {
        StatsComponent->MaxHealth = 500.f;
        StatsComponent->CurrentHealth = 500.f;
        StatsComponent->MaxStamina = 200.f;
        StatsComponent->CurrentStamina = 200.f;
    }

    // 보스 AI 설정
    DetectionRange = 2000.f;
    AttackRange = 300.f;
    LoseTargetRange = 3000.f;
    MoveSpeed = 250.f;
    AttackCooldown = 1.5f;
    MaxAttackPatterns = 4;
}

void ABossEnemy::BeginPlay()
{
    Super::BeginPlay();

    // 페이즈 1 시작
    CurrentPhase = 1;

    // ========================================================================
    // 애니메이션 몽타주 초기화
    // ========================================================================
    auto InitMontage = [](UAnimMontage*& OutMontage, const FString& AnimPath, const char* Name)
    {
        if (!AnimPath.empty())
        {
            UAnimSequence* Anim = UResourceManager::GetInstance().Get<UAnimSequence>(AnimPath);
            if (Anim)
            {
                OutMontage = NewObject<UAnimMontage>();
                OutMontage->SetSourceSequence(Anim);
                UE_LOG("[BossEnemy] %s montage initialized: %s", Name, AnimPath.c_str());
            }
            else
            {
                UE_LOG("[BossEnemy] Failed to find animation: %s", AnimPath.c_str());
            }
        }
    };

    InitMontage(LightComboMontage, LightComboAnimPath, "LightCombo");
    InitMontage(HeavySlamMontage, HeavySlamAnimPath, "HeavySlam");
    InitMontage(ChargeAttackMontage, ChargeAttackAnimPath, "ChargeAttack");
    InitMontage(SpinAttackMontage, SpinAttackAnimPath, "SpinAttack");
}

void ABossEnemy::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // 페이즈 전환 체크
    CheckPhaseTransition();
}

// ============================================================================
// 공격 패턴
// ============================================================================

void ABossEnemy::ExecuteAttackPattern(int PatternIndex)
{
    // 페이즈에 따라 공격 패턴 변경
    if (CurrentPhase >= 2)
    {
        // 페이즈 2에서는 더 강력한 패턴
        PatternIndex = (PatternIndex + 2) % MaxAttackPatterns;
    }

    switch (PatternIndex)
    {
    case 0:
        Attack_LightCombo();
        break;
    case 1:
        Attack_HeavySlam();
        break;
    case 2:
        Attack_ChargeAttack();
        break;
    case 3:
        Attack_SpinAttack();
        break;
    default:
        Attack_LightCombo();
        break;
    }
}

void ABossEnemy::Attack_LightCombo()
{
    FDamageInfo DamageInfo;
    DamageInfo.Instigator = this;
    DamageInfo.Damage = 20.f;
    DamageInfo.DamageType = EDamageType::Light;
    DamageInfo.HitReaction = EHitReaction::Flinch;
    DamageInfo.StaggerDuration = 0.3f;

    HitboxComponent->EnableHitbox(DamageInfo);

    // 콤보 애니메이션 재생
    if (LightComboMontage && GetMesh())
    {
        if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
        {
            AnimInst->SetRootMotionEnabled(bEnableAttackRootMotion);
            AnimInst->SetAnimationCutEndTime(AnimationCutEndTime);
            AnimInst->Montage_Play(LightComboMontage, 0.1f, 0.1f, 1.0f);
        }
    }
}

void ABossEnemy::Attack_HeavySlam()
{
    FDamageInfo DamageInfo;
    DamageInfo.Instigator = this;
    DamageInfo.Damage = 40.f;
    DamageInfo.DamageType = EDamageType::Heavy;
    DamageInfo.HitReaction = EHitReaction::Knockback;
    DamageInfo.StaggerDuration = 0.8f;
    DamageInfo.KnockbackForce = 500.f;

    bHasSuperArmor = true;  // 강공격 중 슈퍼아머
    HitboxComponent->EnableHitbox(DamageInfo);

    // 내려찍기 애니메이션 재생
    if (HeavySlamMontage && GetMesh())
    {
        if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
        {
            AnimInst->SetRootMotionEnabled(bEnableAttackRootMotion);
            AnimInst->SetAnimationCutEndTime(AnimationCutEndTime);
            AnimInst->Montage_Play(HeavySlamMontage, 0.1f, 0.1f, 1.0f);
        }
    }
}

void ABossEnemy::Attack_ChargeAttack()
{
    FDamageInfo DamageInfo;
    DamageInfo.Instigator = this;
    DamageInfo.Damage = 35.f;
    DamageInfo.DamageType = EDamageType::Heavy;
    DamageInfo.HitReaction = EHitReaction::Knockback;
    DamageInfo.StaggerDuration = 0.6f;
    DamageInfo.KnockbackForce = 800.f;

    bHasSuperArmor = true;
    HitboxComponent->EnableHitbox(DamageInfo);

    // 돌진 이동
    if (TargetActor)
    {
        FVector Direction = (TargetActor->GetActorLocation() - GetActorLocation()).GetNormalized();
        Direction.Z = 0.f;
        SetActorLocation(GetActorLocation() + Direction * 500.f);
    }

    // 돌진 애니메이션 재생
    if (ChargeAttackMontage && GetMesh())
    {
        if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
        {
            AnimInst->SetRootMotionEnabled(bEnableAttackRootMotion);
            AnimInst->SetAnimationCutEndTime(AnimationCutEndTime);
            AnimInst->Montage_Play(ChargeAttackMontage, 0.05f, 0.1f, 1.2f);
        }
    }
}

void ABossEnemy::Attack_SpinAttack()
{
    FDamageInfo DamageInfo;
    DamageInfo.Instigator = this;
    DamageInfo.Damage = 25.f;
    DamageInfo.DamageType = EDamageType::Special;
    DamageInfo.HitReaction = EHitReaction::Stagger;
    DamageInfo.StaggerDuration = 0.5f;
    DamageInfo.bCanBeBlocked = false;  // 가드 불가

    bHasSuperArmor = true;
    HitboxComponent->EnableHitbox(DamageInfo);

    // 회전 공격 애니메이션 재생
    if (SpinAttackMontage && GetMesh())
    {
        if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
        {
            AnimInst->SetRootMotionEnabled(bEnableAttackRootMotion);
            AnimInst->SetAnimationCutEndTime(AnimationCutEndTime);
            AnimInst->Montage_Play(SpinAttackMontage, 0.1f, 0.1f, 1.0f);
        }
    }
}

// ============================================================================
// 몽타주 재생 (Lua용)
// ============================================================================

bool ABossEnemy::PlayMontageByName(const FString& MontageName, float BlendIn, float BlendOut, float PlayRate)
{
    UAnimMontage* Montage = nullptr;

    // 몽타주 이름으로 찾기
    if (MontageName == "LightCombo")
        Montage = LightComboMontage;
    else if (MontageName == "HeavySlam")
        Montage = HeavySlamMontage;
    else if (MontageName == "ChargeAttack")
        Montage = ChargeAttackMontage;
    else if (MontageName == "SpinAttack")
        Montage = SpinAttackMontage;

    if (!Montage || !GetMesh())
    {
        UE_LOG("[BossEnemy] PlayMontageByName failed: %s not found", MontageName.c_str());
        return false;
    }

    if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
    {
        AnimInst->SetRootMotionEnabled(bEnableAttackRootMotion);
        AnimInst->SetAnimationCutEndTime(AnimationCutEndTime);
        AnimInst->Montage_Play(Montage, BlendIn, BlendOut, PlayRate);
        UE_LOG("[BossEnemy] Playing montage: %s", MontageName.c_str());
        return true;
    }

    return false;
}

// ============================================================================
// 페이즈 시스템
// ============================================================================

void ABossEnemy::SetPhase(int32 NewPhase)
{
    if (NewPhase == CurrentPhase || NewPhase < 1 || NewPhase > MaxPhase)
    {
        return;
    }

    int32 OldPhase = CurrentPhase;
    CurrentPhase = NewPhase;
    OnPhaseChanged(OldPhase, NewPhase);
}

void ABossEnemy::CheckPhaseTransition()
{
    if (!StatsComponent || CurrentPhase >= MaxPhase)
    {
        return;
    }

    float HealthPercent = StatsComponent->GetHealthPercent();

    // 페이즈 2 진입 조건
    if (CurrentPhase == 1 && HealthPercent <= Phase2HealthThreshold)
    {
        SetPhase(2);
    }
}

void ABossEnemy::OnPhaseChanged(int32 OldPhase, int32 NewPhase)
{
    // 페이즈 변경 시 처리
    if (NewPhase == 2)
    {
        // 페이즈 2: 더 공격적으로
        AttackCooldown = 1.0f;
        MoveSpeed = 350.f;

        // TODO: 페이즈 전환 연출 (포효 애니메이션 등)
    }
}
