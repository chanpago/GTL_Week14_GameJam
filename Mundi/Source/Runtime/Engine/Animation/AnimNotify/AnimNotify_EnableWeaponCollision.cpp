#include "pch.h"
#include "AnimNotify_EnableWeaponCollision.h"
#include "SkeletalMeshComponent.h"
#include "AnimSequenceBase.h"
#include "Actor.h"
#include "Character.h"

IMPLEMENT_CLASS(UAnimNotify_EnableWeaponCollision)

UAnimNotify_EnableWeaponCollision::UAnimNotify_EnableWeaponCollision()
{
    bEnable = true;
}

void UAnimNotify_EnableWeaponCollision::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp)
        return;

    AActor* Owner = MeshComp->GetOwner();
    if (!Owner)
        return;

    // Character로 캐스팅
    ACharacter* Character = Cast<ACharacter>(Owner);
    if (!Character)
    {
        UE_LOG("[AnimNotify_EnableWeaponCollision] Owner is not a Character: %s", Owner->GetName().c_str());
        return;
    }

    // 무기 충돌 활성화/비활성화
    Character->EnableWeaponCollision(bEnable);

    UE_LOG("[AnimNotify_EnableWeaponCollision] Weapon collision %s on %s",
           bEnable ? "enabled" : "disabled",
           Owner->GetName().c_str());
}
