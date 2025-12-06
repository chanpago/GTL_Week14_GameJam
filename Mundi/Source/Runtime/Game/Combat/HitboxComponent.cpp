#include "pch.h"
#include "HitboxComponent.h"
#include "Actor.h"
#include "World.h"
#include "Collision.h"
#include "Source/Runtime/Engine/Physics/PhysScene.h"
#include "Source/Runtime/Game/Player/PlayerCharacter.h"
#include "Renderer.h"
#include "RenderSettings.h"
#include "StaticMeshActor.h"
#include "StaticMeshComponent.h"


UHitboxComponent::UHitboxComponent()
{
    bCanEverTick = true;
    bTickEnabled = true;
    bGenerateOverlapEvents = false;  // SweepBox 사용하므로 오버랩 이벤트 불필요

    // 기본적으로 비활성화 상태로 시작
    bIsActive = false;

    // 기본 히트박스 크기
    BoxExtent = FVector(0.5f, 0.5f, 0.5f);  // 미터 단위 (50cm)


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

    // 디버그 히트 표시 타이머 감소 및 만료된 것 제거
    for (int i = DebugHitInfos.Num() - 1; i >= 0; --i)
    {
        DebugHitInfos[i].RemainingTime -= DeltaTime;
        if (DebugHitInfos[i].RemainingTime <= 0.0f)
        {
            DebugHitInfos.RemoveAt(i);
        }
    }

    if (!bIsActive)
    {
        return;
    }

    // 자동 비활성화 타이머 처리
    if (bUseAutoDisable)
    {
        RemainingDuration -= DeltaTime;
        if (RemainingDuration <= 0.0f)
        {
            DisableHitbox();
            return;
        }
    }

    // SweepBox로 충돌 감지
    PerformSweepCheck();
}

// ============================================================================
// 히트박스 활성화/비활성화
// ============================================================================

void UHitboxComponent::EnableHitbox(const FDamageInfo& InDamageInfo, float InDuration)
{
    CurrentDamageInfo = InDamageInfo;

    // Instigator가 없으면 Owner로 설정
    if (!CurrentDamageInfo.Instigator)
    {
        CurrentDamageInfo.Instigator = OwnerActor;
    }

    ClearHitActors();
    bIsActive = true;

    // 자동 비활성화 설정
    if (InDuration > 0.0f)
    {
        bUseAutoDisable = true;
        RemainingDuration = InDuration;
    }
    else
    {
        bUseAutoDisable = false;
        RemainingDuration = 0.0f;
    }

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
// 충돌 처리
// ============================================================================

void UHitboxComponent::OnOverlapDetected(AActor* OtherActor, const FVector& HitPosition, const FVector& HitNormal)
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

    // 히트 처리 (PhysX에서 받은 정확한 충돌 위치 사용)
    FCombatHitResult Result = ProcessHit(OtherActor, Target, HitPosition, HitNormal);

    // 히트 목록에 추가
    AddHitActor(OtherActor);

    // 콜백 호출
    if (OnHitCallback)
    {
        OnHitCallback(OtherActor, Result);
    }
}

FCombatHitResult UHitboxComponent::ProcessHit(AActor* TargetActor, IDamageable* Target, const FVector& HitPosition, const FVector& HitNormal)
{
    FCombatHitResult Result;
    Result.bWasHit = true;

    // PhysX에서 받은 정확한 충돌 위치 사용
    FDamageInfo DamageToApply = CurrentDamageInfo;
    FVector HitDirection;

    if (TargetActor && OwnerActor)
    {
        FVector OwnerPos = OwnerActor->GetActorLocation();

        // 충돌 방향 계산 (소유자 → 충돌점)
        HitDirection = (HitPosition - OwnerPos).GetNormalized();

        DamageToApply.HitLocation = HitPosition;
        DamageToApply.HitDirection = HitDirection;

        // 결과에 PhysX 충돌 정보 저장
        Result.HitPosition = HitPosition;
        Result.HitNormal = HitNormal;
    }

    // 스킬 공격 여부 판단 (Special 타입이면 스킬)
    bool bIsSkillAttack = (DamageToApply.DamageType == EDamageType::Special);

    // 가드/패리 체크
    if (Target->IsParrying() && DamageToApply.bCanBeParried)
    {
        // 패리 성공!
        Result.bWasParried = true;
        Result.ActualDamage = 0.f;
        Result.AppliedReaction = EHitReaction::None;

        // === 패리 이벤트 발생 ===
        FCombatEvent ParryEvent(ECombatEventType::Parried, OwnerActor, TargetActor);
        ParryEvent.HitPosition = HitPosition;
        ParryEvent.HitDirection = HitDirection;
        ParryEvent.HitNormal = HitNormal;
        ParryEvent.DamageType = DamageToApply.DamageType;
        ParryEvent.bIsSkillAttack = bIsSkillAttack;
        OnCombatEvent.Broadcast(ParryEvent);

        // 스킬 패리면 추가 이벤트
        if (bIsSkillAttack)
        {
            FCombatEvent SkillParryEvent(ECombatEventType::SkillParried, OwnerActor, TargetActor);
            SkillParryEvent.HitPosition = HitPosition;
            SkillParryEvent.HitDirection = HitDirection;
            SkillParryEvent.HitNormal = HitNormal;
            OnCombatEvent.Broadcast(SkillParryEvent);
        }

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

        // === 가드 이벤트 발생 ===
        FCombatEvent BlockEvent(ECombatEventType::BlockedHit, OwnerActor, TargetActor);
        BlockEvent.HitPosition = HitPosition;
        BlockEvent.HitDirection = HitDirection;
        BlockEvent.HitNormal = HitNormal;
        BlockEvent.Damage = DamageToApply.Damage;
        BlockEvent.ActualDamage = Result.ActualDamage;
        BlockEvent.DamageType = DamageToApply.DamageType;
        OnCombatEvent.Broadcast(BlockEvent);

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

    // === 일반 히트 이벤트 발생 ===
    ECombatEventType HitEventType = bIsSkillAttack ? ECombatEventType::SkillHit : ECombatEventType::NormalHit;
    FCombatEvent HitEvent(HitEventType, OwnerActor, TargetActor);
    HitEvent.HitPosition = HitPosition;
    HitEvent.HitDirection = HitDirection;
    HitEvent.HitNormal = HitNormal;  // PhysX에서 받은 정확한 법선
    HitEvent.Damage = DamageToApply.Damage;
    HitEvent.ActualDamage = Result.ActualDamage;
    HitEvent.DamageType = DamageToApply.DamageType;
    HitEvent.bIsSkillAttack = bIsSkillAttack;
    OnCombatEvent.Broadcast(HitEvent);

    return Result;
}

// ============================================================================
// SweepBox 충돌 감지
// ============================================================================

void UHitboxComponent::PerformSweepCheck()
{
    if (!OwnerActor)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FPhysScene* PhysScene = World->GetPhysScene();
    if (!PhysScene)
    {
        return;
    }

    // 히트박스 위치와 회전
    FVector BoxCenter = GetWorldLocation();
    FQuat BoxRotation = GetWorldRotation();

    // Sweep 방향 (약간 앞으로) - 최소 거리로 현재 위치에서 충돌 체크
    FVector SweepDirection = OwnerActor->GetActorForward();
    float SweepDistance = 0.01f;  // 1cm
    FVector SweepEnd = BoxCenter + SweepDirection * SweepDistance;

    // SweepBox 실행
    FHitResult HitResult;
    bool bHit = PhysScene->SweepBox(
        BoxCenter,
        SweepEnd,
        BoxExtent,
        BoxRotation,
        HitResult,
        OwnerActor  // 자기 자신 무시
    );

    if (bHit && HitResult.HitActor)
    {
        // 플레이어만 충돌 처리
        APlayerCharacter* Player = Cast<APlayerCharacter>(HitResult.HitActor);
        if (!Player)
        {
            return;
        }

        // 디버그용 - 히트 정보 저장

            FDebugHitInfo HitInfo;
            HitInfo.Position = HitResult.ImpactPoint;
            HitInfo.Normal = HitResult.ImpactNormal;
            HitInfo.RemainingTime = 3.0f;  // 3초 동안 표시
            DebugHitInfos.Add(HitInfo);

        // 충돌 위치에 StaticMeshActor 스폰
        // 히트박스(칼) 위치와 피해자(플레이어) 위치 사이, 피해자 몸쪽에 스폰
        FVector HitboxPos = BoxCenter;  // 히트박스(칼) 위치
        FVector VictimPos = Player->GetActorLocation();  // 피해자 위치

        // 히트박스에서 피해자 방향으로 피해자 몸쪽에 스폰 (피해자 위치의 80% 지점)
        FVector SpawnLocation = FVector::Lerp(HitboxPos, VictimPos, 0.8f);

        FTransform SpawnTransform;
        SpawnTransform.Translation = SpawnLocation;
        AStaticMeshActor* HitMarker = World->SpawnActor<AStaticMeshActor>(SpawnTransform);
        if (HitMarker)
        {
            HitMarker->GetStaticMeshComponent()->SetStaticMesh(GDataDir + "/Model/Sphere8.obj");
            HitMarker->SetActorScale(FVector(0.1f, 0.1f, 0.1f));  // 작은 구체로 표시
        }

        // 정확한 충돌 위치와 법선으로 처리
        OnOverlapDetected(HitResult.HitActor, HitResult.ImpactPoint, HitResult.ImpactNormal);
    }
}

// ============================================================================
// 디버그 렌더링
// ============================================================================

void UHitboxComponent::RenderDebugVolume(URenderer* Renderer) const
{
    if (!bIsActive)
    {
        return;
    }

    // 활성화 상태일 때만 히트박스 표시
    if (bIsActive)
    {
        const FVector Extent = BoxExtent;
        const FTransform WorldTransform = GetWorldTransform();

        TArray<FVector> StartPoints;
        TArray<FVector> EndPoints;
        TArray<FVector4> Colors;

        // 박스의 8개 꼭짓점 (로컬)
        FVector local[8] = {
            {-Extent.X, -Extent.Y, -Extent.Z}, {+Extent.X, -Extent.Y, -Extent.Z},
            {-Extent.X, +Extent.Y, -Extent.Z}, {+Extent.X, +Extent.Y, -Extent.Z},
            {-Extent.X, -Extent.Y, +Extent.Z}, {+Extent.X, -Extent.Y, +Extent.Z},
            {-Extent.X, +Extent.Y, +Extent.Z}, {+Extent.X, +Extent.Y, +Extent.Z},
        };

        // 월드 좌표로 변환
        FVector WorldSpace[8];
        for (int i = 0; i < 8; i++)
        {
            WorldSpace[i] = WorldTransform.TransformPosition(local[i]);
        }

        // 히트박스 색상 - 활성화 시 빨간색
        FVector4 HitboxColor = FVector4(1.0f, 0.0f, 0.0f, 1.0f);

        // 12개의 엣지
        static const int Edge[12][2] = {
            {0,1},{1,3},{3,2},{2,0}, // bottom
            {4,5},{5,7},{7,6},{6,4}, // top
            {0,4},{1,5},{2,6},{3,7}  // verticals
        };

        for (int i = 0; i < 12; ++i)
        {
            StartPoints.Add(WorldSpace[Edge[i][0]]);
            EndPoints.Add(WorldSpace[Edge[i][1]]);
            Colors.Add(HitboxColor);
        }

        Renderer->AddLines(StartPoints, EndPoints, Colors);
    }

    // 모든 히트 포인트 표시
    if (DebugHitInfos.Num() > 0)
    {
        TArray<FVector> HitStartPoints;
        TArray<FVector> HitEndPoints;
        TArray<FVector4> HitColors;

        // 히트 포인트에 십자가 표시 (노란색)
        FVector4 HitPointColor = FVector4(1.0f, 1.0f, 0.0f, 1.0f);
        float CrossSize = 0.1f;  // 10cm

        // 히트 노말 방향 표시 (초록색)
        FVector4 NormalColor = FVector4(0.0f, 1.0f, 0.0f, 1.0f);
        float NormalLength = 0.3f;  // 30cm

        for (const FDebugHitInfo& HitInfo : DebugHitInfos)
        {
            const FVector& Pos = HitInfo.Position;
            const FVector& Normal = HitInfo.Normal;

            // X축 십자
            HitStartPoints.Add(Pos - FVector(CrossSize, 0, 0));
            HitEndPoints.Add(Pos + FVector(CrossSize, 0, 0));
            HitColors.Add(HitPointColor);

            // Y축 십자
            HitStartPoints.Add(Pos - FVector(0, CrossSize, 0));
            HitEndPoints.Add(Pos + FVector(0, CrossSize, 0));
            HitColors.Add(HitPointColor);

            // Z축 십자
            HitStartPoints.Add(Pos - FVector(0, 0, CrossSize));
            HitEndPoints.Add(Pos + FVector(0, 0, CrossSize));
            HitColors.Add(HitPointColor);

            // 노말 방향 표시
            HitStartPoints.Add(Pos);
            HitEndPoints.Add(Pos + Normal * NormalLength);
            HitColors.Add(NormalColor);
        }

        Renderer->AddLines(HitStartPoints, HitEndPoints, HitColors);
    }
}
