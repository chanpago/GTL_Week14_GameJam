#pragma once
#include "ParticleSystemComponent.h"

// Specialized particle component spawned by animation notifies to control forced lifetime.
class UAnimNotifyParticleComponent : public UParticleSystemComponent
{
public:
    DECLARE_CLASS(UAnimNotifyParticleComponent, UParticleSystemComponent)

    UAnimNotifyParticleComponent();

    void TickComponent(float DeltaTime) override;

    void SetForcedLifeTime(float InSeconds);

private:
    float ForcedLifeTime = -1.0f;
    bool bForcedFinishTriggered = false;
};