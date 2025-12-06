#pragma once

#include "AnimNotify.h"
#include "Vector.h"
#include "Name.h"

class UParticleSystem;

// Simple notify that spawns a particle system either on the owning actor or as a transient actor in the world.
class UAnimNotify_PlayParticle : public UAnimNotify
{
public:
    DECLARE_CLASS(UAnimNotify_PlayParticle, UAnimNotify)

    UAnimNotify_PlayParticle();

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

    void SetEventDuration(float InDuration) { CachedDuration = InDuration; }
    float GetEventDuration() const { return CachedDuration; }
    float GetResolvedLifetime() const;

    // Editable properties (exposed through editor UI later)
    UParticleSystem* ParticleSystem = nullptr;
    FVector LocationOffset = FVector::Zero();
    FVector RotationOffset = FVector::Zero(); // Degrees (ZYX order)
    FVector ScaleOffset = FVector::One();
    FName AttachBoneName; // Optional bone sampling (only evaluated at spawn time)
    bool bAttachToOwner = false; // true -> attach component to owning actor, false -> spawn transient actor
    float LifeTime = 0.0f; // Optional forced life span (seconds). <= 0 uses notify duration / particle asset

private:
    float CachedDuration = 0.0f;
};
