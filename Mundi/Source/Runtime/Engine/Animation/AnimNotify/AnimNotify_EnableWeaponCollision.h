#pragma once
#include "AnimNotify.h"

// ============================================================================
// UAnimNotify_EnableWeaponCollision - 무기 충돌 활성화 노티파이
// ============================================================================
class UAnimNotify_EnableWeaponCollision : public UAnimNotify
{
public:
    DECLARE_CLASS(UAnimNotify_EnableWeaponCollision, UAnimNotify)

    UAnimNotify_EnableWeaponCollision();

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

    // 충돌 활성화 여부
    UPROPERTY(EditAnywhere, Category = "Weapon")
    bool bEnable = true;
};
