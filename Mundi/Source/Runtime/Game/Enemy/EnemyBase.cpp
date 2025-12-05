#include "pch.h"
#include "EnemyBase.h"
#include "StatsComponent.h"
#include "HitboxComponent.h"
#include "World.h"
#include "PlayerController.h"



AEnemyBase::AEnemyBase()
{
    // 스탯 컴포넌트
    StatsComponent = CreateDefaultSubobject<UStatsComponent>("StatsComponent");
    StatsComponent->MaxHealth = 100.f;
    StatsComponent->CurrentHealth = 100.f;

    // 히트박스 컴포넌트
    HitboxComponent = CreateDefaultSubobject<UHitboxComponent>("HitboxComponent");
    HitboxComponent->SetBoxExtent(FVector(60.f, 60.f, 60.f));
}

void AEnemyBase::BeginPlay()
{
    Super::BeginPlay();

    // 델리게이트 바인딩
    if (StatsComponent)
    {
        //StatsComponent->OnDeath.AddDynamic(this, &AEnemyBase::HandleDeath);
    }

    // 히트박스 소유자 설정
    if (HitboxComponent)
    {
        HitboxComponent->SetOwnerActor(this);
    }
}

void AEnemyBase::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (AIState != EEnemyAIState::Dead)
    {
        UpdateAI(DeltaSeconds);
    }
}

// ============================================================================
// AI 로직
// ============================================================================

void AEnemyBase::UpdateAI(float DeltaTime)
{
    switch (AIState)
    {
    case EEnemyAIState::Idle:
        UpdateIdle(DeltaTime);
        break;
    case EEnemyAIState::Chase:
        UpdateChase(DeltaTime);
        break;
    case EEnemyAIState::Attack:
        UpdateAttack(DeltaTime);
        break;
    case EEnemyAIState::Stagger:
        UpdateStagger(DeltaTime);
        break;
    default:
        break;
    }

    // 공격 쿨타임 업데이트
    if (AttackTimer > 0.f)
    {
        AttackTimer -= DeltaTime;
    }
}

void AEnemyBase::SetAIState(EEnemyAIState NewState)
{
    if (AIState == NewState)
    {
        return;
    }

    EEnemyAIState OldState = AIState;
    AIState = NewState;

    // 상태 전환 시 처리
    if (OldState == EEnemyAIState::Attack)
    {
        bIsAttacking = false;
        HitboxComponent->DisableHitbox();
    }
}

void AEnemyBase::UpdateIdle(float DeltaTime)
{
    // 플레이어 감지 시 추적 시작
    if (DetectPlayer())
    {
        SetAIState(EEnemyAIState::Chase);
    }
}

void AEnemyBase::UpdateChase(float DeltaTime)
{
    if (!TargetActor)
    {
        SetAIState(EEnemyAIState::Idle);
        return;
    }

    // 거리 체크
    float Distance = (TargetActor->GetActorLocation() - GetActorLocation()).Size();

    // 너무 멀면 타겟 잃음
    if (Distance > LoseTargetRange)
    {
        TargetActor = nullptr;
        SetAIState(EEnemyAIState::Idle);
        return;
    }

    // 공격 범위 안이면 공격
    if (Distance <= AttackRange && AttackTimer <= 0.f)
    {
        StartAttack();
        return;
    }

    // 타겟 방향으로 이동
    LookAtTarget(DeltaTime);
    MoveToTarget(DeltaTime);
}

void AEnemyBase::UpdateAttack(float DeltaTime)
{
    // 공격 애니메이션이 끝나면 상태 전환
    // TODO: 애니메이션 완료 체크
    // 임시로 타이머 사용
    if (!bIsAttacking)
    {
        SetAIState(EEnemyAIState::Chase);
    }
}

void AEnemyBase::UpdateStagger(float DeltaTime)
{
    StaggerTimer -= DeltaTime;
    if (StaggerTimer <= 0.f)
    {
        SetAIState(EEnemyAIState::Chase);
    }
}

// ============================================================================
// 감지
// ============================================================================

bool AEnemyBase::DetectPlayer()
{
    // 월드에서 플레이어 찾기
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    // TODO: 플레이어 캐릭터 찾는 로직
    // 임시: 태그로 찾기 또는 PlayerController에서 가져오기
    // APlayerController* PC = World->GetFirstPlayerController();
    // if (PC && PC->GetPawn())
    // {
    //     float Distance = (PC->GetPawn()->GetActorLocation() - GetActorLocation()).Length();
    //     if (Distance <= DetectionRange)
    //     {
    //         TargetActor = PC->GetPawn();
    //         return true;
    //     }
    // }

    return false;
}

bool AEnemyBase::IsPlayerInAttackRange()
{
    if (!TargetActor)
    {
        return false;
    }

    float Distance = (TargetActor->GetActorLocation() - GetActorLocation()).Size();
    return Distance <= AttackRange;
}

void AEnemyBase::LookAtTarget(float DeltaTime)
{
    if (!TargetActor)
    {
        return;
    }

    FVector Direction = TargetActor->GetActorLocation() - GetActorLocation();
    Direction.Z = 0.f; // 수평 방향만

    if (Direction.SizeSquared() > 0.01f)
    {
        Direction = Direction.GetNormalized();
        // TODO: 부드러운 회전
        // FQuat TargetRotation = FQuat::LookAt(Direction, FVector::UpVector);
        // SetActorRotation(FQuat::Slerp(GetActorRotation(), TargetRotation, RotationSpeed * DeltaTime));
    }
}

void AEnemyBase::MoveToTarget(float DeltaTime)
{
    if (!TargetActor)
    {
        return;
    }

    FVector Direction = TargetActor->GetActorLocation() - GetActorLocation();
    float Distance = Direction.Size();

    // 공격 범위보다 가까우면 이동 안 함
    if (Distance <= AttackRange * 0.8f)
    {
        return;
    }

    Direction = Direction.GetNormalized();
    FVector NewLocation = GetActorLocation() + Direction * MoveSpeed * DeltaTime;
    SetActorLocation(NewLocation);
}

// ============================================================================
// 공격
// ============================================================================

void AEnemyBase::StartAttack()
{
    SetAIState(EEnemyAIState::Attack);
    bIsAttacking = true;
    AttackTimer = AttackCooldown;

    // 패턴 선택 (랜덤 또는 순차)
    ExecuteAttackPattern(CurrentAttackPattern);
    CurrentAttackPattern = (CurrentAttackPattern + 1) % MaxAttackPatterns;
}

void AEnemyBase::ExecuteAttackPattern(int32 PatternIndex)
{
    FDamageInfo DamageInfo;
    DamageInfo.Instigator = this;

    switch (PatternIndex)
    {
    case 0: // 약공격
        DamageInfo.Damage = 15.f;
        DamageInfo.DamageType = EDamageType::Light;
        DamageInfo.HitReaction = EHitReaction::Flinch;
        DamageInfo.StaggerDuration = 0.3f;
        break;

    case 1: // 강공격
        DamageInfo.Damage = 30.f;
        DamageInfo.DamageType = EDamageType::Heavy;
        DamageInfo.HitReaction = EHitReaction::Stagger;
        DamageInfo.StaggerDuration = 0.6f;
        bHasSuperArmor = true; // 강공격 중 슈퍼아머
        break;

    default:
        DamageInfo.Damage = 10.f;
        DamageInfo.DamageType = EDamageType::Light;
        break;
    }

    HitboxComponent->EnableHitbox(DamageInfo);

    // TODO: 공격 애니메이션 재생
}

// ============================================================================
// IDamageable 구현
// ============================================================================

float AEnemyBase::TakeDamage(const FDamageInfo& DamageInfo)
{
    if (!CanBeHit())
    {
        return 0.f;
    }

    StatsComponent->ApplyDamage(DamageInfo.Damage);

    // 슈퍼아머가 없으면 피격 반응
    if (!bHasSuperArmor)
    {
        OnHitReaction(DamageInfo.HitReaction, DamageInfo);
    }

    return DamageInfo.Damage;
}

bool AEnemyBase::IsAlive() const
{
    return StatsComponent && StatsComponent->IsAlive();
}

bool AEnemyBase::CanBeHit() const
{
    return IsAlive() && AIState != EEnemyAIState::Dead;
}

void AEnemyBase::OnHitReaction(EHitReaction Reaction, const FDamageInfo& DamageInfo)
{
    if (Reaction == EHitReaction::None)
    {
        return;
    }

    // 공격 중단
    bIsAttacking = false;
    bHasSuperArmor = false;
    HitboxComponent->DisableHitbox();

    // 경직 상태로 전환
    SetAIState(EEnemyAIState::Stagger);
    StaggerTimer = DamageInfo.StaggerDuration;

    // TODO: 피격 애니메이션 재생

    // 넉백
    if (Reaction == EHitReaction::Knockback && DamageInfo.KnockbackForce > 0.f)
    {
        FVector KnockbackDir = DamageInfo.HitDirection * DamageInfo.KnockbackForce;
        AddActorWorldLocation(KnockbackDir * 0.1f);
    }
}

void AEnemyBase::OnDeath()
{
    SetAIState(EEnemyAIState::Dead);
    HitboxComponent->DisableHitbox();

    // TODO: 사망 애니메이션/래그돌
    // TODO: 일정 시간 후 Destroy
}

void AEnemyBase::HandleDeath()
{
    OnDeath();
}
