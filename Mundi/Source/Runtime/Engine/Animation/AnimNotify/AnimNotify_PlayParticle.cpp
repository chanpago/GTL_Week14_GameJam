#include "pch.h"
#include "AnimNotify_PlayParticle.h"

#include "Source/Runtime/Engine/Components/SkeletalMeshComponent.h"
#include "Source/Runtime/Engine/Components/AnimNotifyParticleComponent.h"
#include "Source/Runtime/Engine/Animation/AnimSequenceBase.h"
#include "Source/Runtime/Engine/GameFramework/EmptyActor.h"
#include "Source/Runtime/Engine/GameFramework/World.h"
#include "Source/Runtime/Core/Containers/UEContainer.h"

IMPLEMENT_CLASS(UAnimNotify_PlayParticle)

UAnimNotify_PlayParticle::UAnimNotify_PlayParticle()
{
}

float UAnimNotify_PlayParticle::GetResolvedLifetime() const
{
    if (LifeTime > 0.0f)
    {
        return LifeTime;
    }

    if (CachedDuration > 0.0f)
    {
        return CachedDuration;
    }

    return -1.0f;
}

void UAnimNotify_PlayParticle::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp || !ParticleSystem)
    {
        return;
    }

    UWorld* World = MeshComp->GetWorld();
    if (!World)
    {
        return;
    }

    FTransform BaseTransform = MeshComp->GetWorldTransform();
    if (AttachBoneName.IsValid())
    {
        const int32 BoneIndex = MeshComp->GetBoneIndexByName(AttachBoneName);
        if (BoneIndex >= 0)
        {
            BaseTransform = MeshComp->GetBoneWorldTransform(BoneIndex);
        }
    }

    const FQuat OffsetRotation = FQuat::MakeFromEulerZYX(RotationOffset);
    const FTransform OffsetTransform(LocationOffset, OffsetRotation, ScaleOffset);
    const FTransform SpawnTransform = BaseTransform.GetWorldTransform(OffsetTransform);

    UAnimNotifyParticleComponent* ParticleComponent = nullptr;
    AActor* TargetActor = nullptr;

    if (bAttachToOwner)
    {
        AActor* OwnerActor = MeshComp->GetOwner();
        if (!OwnerActor)
        {
            return;
        }

        ParticleComponent = Cast<UAnimNotifyParticleComponent>(OwnerActor->AddNewComponent(UAnimNotifyParticleComponent::StaticClass(), MeshComp));
        TargetActor = OwnerActor;
    }
    else
    {
        TargetActor = World->SpawnActor<AEmptyActor>(SpawnTransform);
        if (!TargetActor)
        {
            return;
        }

        ParticleComponent = Cast<UAnimNotifyParticleComponent>(TargetActor->AddNewComponent(UAnimNotifyParticleComponent::StaticClass()));
    }

    if (!ParticleComponent)
    {
        if (!bAttachToOwner && TargetActor)
        {
            TargetActor->Destroy();
        }
        return;
    }

    ParticleComponent->SetTemplate(ParticleSystem);
    ParticleComponent->SetWorldTransform(SpawnTransform);
    ParticleComponent->ResetAndActivate();

    const float DesiredLifetime = GetResolvedLifetime();
    if (DesiredLifetime > 0.0f)
    {
        ParticleComponent->SetForcedLifeTime(DesiredLifetime);
    }
    else
    {
        ParticleComponent->SetForcedLifeTime(-1.0f);
    }

    const bool bDestroyActorOnFinish = !bAttachToOwner;
    TWeakObjectPtr<AActor> TargetActorWeak(TargetActor);
    ParticleComponent->OnParticleSystemFinished.Add([TargetActorWeak, bDestroyActorOnFinish](UParticleSystemComponent* FinishedComp)
    {
        AActor* OwnerActor = TargetActorWeak.Get();
        if (bDestroyActorOnFinish)
        {
            if (OwnerActor && !OwnerActor->IsPendingDestroy())
            {
                OwnerActor->Destroy();
            }
        }
        else if (OwnerActor && FinishedComp)
        {
            if (!OwnerActor->IsPendingDestroy())
            {
                OwnerActor->RemoveOwnedComponent(FinishedComp);
            }
        }
        else if (FinishedComp)
        {
            FinishedComp->DestroyComponent();
        }
    });
}
