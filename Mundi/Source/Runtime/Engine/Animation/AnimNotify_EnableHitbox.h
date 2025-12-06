#pragma once
#include "AnimNotify.h"
#include "CombatTypes.h"

class UHitboxComponent;

// ============================================================================
// UAnimNotify_EnableHitbox - 히트박스 활성화 노티파이
// ============================================================================
class UAnimNotify_EnableHitbox : public UAnimNotify
{
public:
    DECLARE_CLASS(UAnimNotify_EnableHitbox, UAnimNotify)

    UAnimNotify_EnableHitbox();

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

    // 데미지 설정
    UPROPERTY(EditAnywhere, Category = "Damage")
    float Damage = 10.0f;

    UPROPERTY(EditAnywhere, Category = "Damage")
    FString DamageType = "Light";

    // 히트박스 크기 (미터 단위)
    UPROPERTY(EditAnywhere, Category = "Hitbox")
    FVector HitboxExtent = FVector(1.5f, 1.0f, 1.0f);

    // 히트박스 오프셋 (캐릭터 로컬 좌표)
    // X: 전방(+)/후방(-), Y: 우측(+)/좌측(-), Z: 위(+)/아래(-)
    UPROPERTY(EditAnywhere, Category = "Hitbox")
    FVector HitboxOffset = FVector(2.0f, 0.0f, 1.0f);

    // 히트박스 지속 시간 (초)
    UPROPERTY(EditAnywhere, Category = "Hitbox")
    float Duration = 0.2f;

private:
    FVector CalculateWorldPosition(AActor* Owner) const;
};
