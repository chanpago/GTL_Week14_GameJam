#include "pch.h"
#include "HitboxComponent.h"
#include "Actor.h"
#include "World.h"


UHitboxComponent::UHitboxComponent()
{
    bCanEverTick = true;
    bTickEnabled = true;
    bGenerateOverlapEvents = true;

    // 기본적으로 비활성화 상태로 시작
    bIsActive = false;

    // 기본 히트박스 크기
    BoxExtent = FVector(50.f, 50.f, 50.f);
}

void UHitboxComponent::BeginPlay()
{
    Super::BeginPlay();

    // 소유자 자동 설정
    if (!OwnerActor)
    {
        OwnerActor = GetOwner();
    }

    // 시작 시 비활성화
    DisableHitbox();
}

void UHitboxComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    if (!bIsActive)
    {
        return;
    }

    // 현재 오버랩 중인 액터들 체크
    const TArray<FOverlapInfo>& Overlaps = GetOverlapInfos();
    for (const FOverlapInfo& Info : Overlaps)
    {
        if (Info.OtherActor && Info.OtherActor != OwnerActor)
        {
            OnOverlapDetected(Info.OtherActor);
        }
    }
}

// ============================================================================
// 히트박스 활성화/비활성화
// ============================================================================

void UHitboxComponent::EnableHitbox(const FDamageInfo& InDamageInfo)
{
    CurrentDamageInfo = InDamageInfo;

    // Instigator가 없으면 Owner로 설정
    if (!CurrentDamageInfo.Instigator)
    {
        CurrentDamageInfo.Instigator = OwnerActor;
    }

    ClearHitActors();
    bIsActive = true;

    // 충돌 활성화
    SetActive(true);
    SetGenerateOverlapEvents(true);
}

void UHitboxComponent::DisableHitbox()
{
    bIsActive = false;

    // 충돌 비활성화 (선택적)
    // SetActive(false);
    // SetGenerateOverlapEvents(false);
}

// ============================================================================
// 히트 액터 관리
// ============================================================================

void UHitboxComponent::ClearHitActors()
{
    HitActors.Empty();
}

bool UHitboxComponent::HasAlreadyHit(AActor* Actor) const
{
    return HitActors.Contains(Actor);
}

void UHitboxComponent::AddHitActor(AActor* Actor)
{
    if (Actor && !HasAlreadyHit(Actor))
    {
        HitActors.Add(Actor);
    }
}

// ============================================================================
// 오버랩 처리
// ============================================================================

void UHitboxComponent::OnOverlapDetected(AActor* OtherActor)
{
    if (!bIsActive || !OtherActor)
    {
        return;
    }

    // 자기 자신은 제외
    if (OtherActor == OwnerActor)
    {
        return;
    }

    // 이미 맞은 액터는 제외
    if (HasAlreadyHit(OtherActor))
    {
        return;
    }

    // IDamageable 인터페이스 확인
    IDamageable* Target = GetDamageable(OtherActor);
    if (!Target)
    {
        return;
    }

    // 피격 가능 상태 확인
    if (!Target->CanBeHit())
    {
        return;
    }

    // 히트 처리
    FHitResult Result = ProcessHit(OtherActor, Target);

    // 히트 목록에 추가
    AddHitActor(OtherActor);

    // 콜백 호출
    if (OnHitCallback)
    {
        OnHitCallback(OtherActor, Result);
    }
}

FHitResult UHitboxComponent::ProcessHit(AActor* TargetActor, IDamageable* Target)
{
    FHitResult Result;
    Result.bWasHit = true;

    // 히트 위치/방향 계산
    FDamageInfo DamageToApply = CurrentDamageInfo;

    if (TargetActor && OwnerActor)
    {
        FVector TargetPos = TargetActor->GetActorLocation();
        FVector OwnerPos = OwnerActor->GetActorLocation();

        DamageToApply.HitLocation = TargetPos;
        DamageToApply.HitDirection = (TargetPos - OwnerPos).GetNormalized();
    }

    // 가드/패리 체크
    if (Target->IsParrying() && DamageToApply.bCanBeParried)
    {
        // 패리 성공!
        Result.bWasParried = true;
        Result.ActualDamage = 0.f;
        Result.AppliedReaction = EHitReaction::None;

        // 공격자에게 패리당함 반응 적용
        if (IDamageable* AttackerDamageable = GetDamageable(OwnerActor))
        {
            FDamageInfo ParryDamage;
            ParryDamage.Instigator = TargetActor;
            ParryDamage.DamageType = EDamageType::Parried;
            ParryDamage.HitReaction = EHitReaction::Stagger;
            ParryDamage.StaggerDuration = 1.5f;
            ParryDamage.Damage = 0.f;

            AttackerDamageable->OnHitReaction(EHitReaction::Stagger, ParryDamage);
        }

        return Result;
    }
    else if (Target->IsBlocking() && DamageToApply.bCanBeBlocked)
    {
        // 가드 성공
        Result.bWasBlocked = true;
        Result.ActualDamage = DamageToApply.Damage * 0.2f; // 가드 시 20% 데미지
        Result.AppliedReaction = EHitReaction::Flinch;

        // 감소된 데미지 적용
        FDamageInfo BlockedDamage = DamageToApply;
        BlockedDamage.Damage = Result.ActualDamage;
        BlockedDamage.HitReaction = EHitReaction::Flinch;

        Target->TakeDamage(BlockedDamage);
        return Result;
    }

    // 일반 히트
    Result.ActualDamage = Target->TakeDamage(DamageToApply);
    Result.AppliedReaction = DamageToApply.HitReaction;

    return Result;
}
