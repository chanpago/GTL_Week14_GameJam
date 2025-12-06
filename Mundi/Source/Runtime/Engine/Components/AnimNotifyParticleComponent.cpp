#include "pch.h"
#include "AnimNotifyParticleComponent.h"

IMPLEMENT_CLASS(UAnimNotifyParticleComponent)

UAnimNotifyParticleComponent::UAnimNotifyParticleComponent()
{
}

void UAnimNotifyParticleComponent::SetForcedLifeTime(float InSeconds)
{
    ForcedLifeTime = InSeconds;
    bForcedFinishTriggered = false;
    if (ForcedLifeTime <= 0.0f)
    {
        ForcedLifeTime = -1.0f;
    }
}

void UAnimNotifyParticleComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    if (ForcedLifeTime < 0.0f || bForcedFinishTriggered)
    {
        return;
    }

    ForcedLifeTime -= DeltaTime;
    if (ForcedLifeTime > 0.0f)
    {
        return;
    }

    ForcedLifeTime = -1.0f;
    bForcedFinishTriggered = true;

    bSuppressSpawning = true;
    DeactivateSystem();
    DestroyParticles();
    SetActive(false);
    SetTickEnabled(false);

    OnParticleSystemFinished.Broadcast(this);
}
