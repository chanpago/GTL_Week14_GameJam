#include "pch.h"
#include "AnimNotify_EnableHitbox.h"
#include "SkeletalMeshComponent.h"
#include "AnimSequenceBase.h"
#include "HitboxComponent.h"
#include "Actor.h"

namespace
{
    EDamageType StringToDamageType(const FString& Str)
    {
        if (Str == "Heavy") return EDamageType::Heavy;
        if (Str == "Special") return EDamageType::Special;
        if (Str == "Parried") return EDamageType::Parried;
        return EDamageType::Light; // 기본값
    }
}

IMPLEMENT_CLASS(UAnimNotify_EnableHitbox)

UAnimNotify_EnableHitbox::UAnimNotify_EnableHitbox()
{
    Damage = 10.0f;
    DamageType = "Light";
    HitboxExtent = FVector(1.5f, 1.0f, 1.0f);
    HitboxOffset = FVector(2.0f, 0.0f, 1.0f);
    Duration = 0.2f;
}

void UAnimNotify_EnableHitbox::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp)
        return;

    AActor* Owner = MeshComp->GetOwner();
    if (!Owner)
        return;

    // 히트박스 컴포넌트 찾기
    UHitboxComponent* Hitbox = Cast<UHitboxComponent>(
        Owner->GetComponent(UHitboxComponent::StaticClass())
    );

    if (!Hitbox)
    {
        UE_LOG("[AnimNotify_EnableHitbox] HitboxComponent not found on %s", Owner->GetName().c_str());
        return;
    }

    // 히트박스 크기 설정
    Hitbox->BoxExtent = HitboxExtent;

    // 히트박스 위치 설정 (캐릭터 회전 고려)
    FVector WorldPos = CalculateWorldPosition(Owner);
    Hitbox->SetWorldLocation(WorldPos);

    // 히트박스 회전을 캐릭터 회전과 동일하게 설정
    Hitbox->SetWorldRotation(Owner->GetActorRotation());

    // 데미지 정보 설정 및 활성화
    FDamageInfo DamageInfo;
    DamageInfo.Damage = Damage;
    DamageInfo.DamageType = StringToDamageType(DamageType);
    DamageInfo.Instigator = Owner;

    Hitbox->EnableHitbox(DamageInfo, Duration);

    UE_LOG("[AnimNotify_EnableHitbox] Hitbox enabled - Damage: %.1f, Type: %s, Duration: %.2fs", Damage, DamageType.c_str(), Duration);
}

FVector UAnimNotify_EnableHitbox::CalculateWorldPosition(AActor* Owner) const
{
    if (!Owner)
        return FVector();

    // 캐릭터의 Forward/Right 벡터를 사용해 로컬 오프셋을 월드 좌표로 변환
    FVector Forward = Owner->GetActorForward();
    FVector Right = Owner->GetActorRight();

    FVector OwnerLocation = Owner->GetActorLocation();

    // 로컬 오프셋 적용: X=전방, Y=우측, Z=위
    // 엔진에서 Forward=Y축, Right=X축이므로 X(전방)에 Forward, Y(우측)에 Right 적용
    FVector WorldOffset = Forward * HitboxOffset.X - Right * HitboxOffset.Y + FVector(0, 0, HitboxOffset.Z);

    return OwnerLocation + WorldOffset;
}
