#pragma once
#include "ParticleModule.h"

// Spiral direction enum
enum class ESpiralDirection : uint8
{
    Clockwise,          // Rotates clockwise when viewed from above (negative angular velocity)
    CounterClockwise,   // Rotates counter-clockwise when viewed from above (positive angular velocity)
    Random              // Randomly choose direction per particle
};

// Per-particle payload for spiral motion
struct FSpiralPayload
{
    FVector CenterPoint;    // The center of the circular orbit (particle's spawn location)
    float CurrentAngle;     // Current angle in radians
    float DirectionMult;    // 1.0 for CCW, -1.0 for CW
};

// Spiral/Circular Motion Particle Module
// Makes particles orbit around their spawn point in a circular pattern
class UParticleModuleSpiral : public UParticleModule
{
    DECLARE_CLASS(UParticleModuleSpiral, UParticleModule)
public:
    UParticleModuleSpiral();

    // ============================================================
    // Module Properties
    // ============================================================

    // Radius of the circular motion
    FRawDistributionFloat Radius = FRawDistributionFloat(50.0f);

    // Angular velocity in degrees per second
    // Higher values = faster rotation
    FRawDistributionFloat AngularVelocity = FRawDistributionFloat(180.0f);

    // Direction of rotation
    ESpiralDirection Direction = ESpiralDirection::CounterClockwise;

    // The axis around which particles orbit (normalized internally)
    // Default is Z-up, meaning particles orbit in the XY plane
    FVector SpiralAxis = FVector(0.0f, 0.0f, 1.0f);

    // Starting angle offset in degrees (allows spiral patterns when combined with spawn timing)
    FRawDistributionFloat StartAngle = FRawDistributionFloat(0.0f);

    // If true, radius expands over particle lifetime (creates outward spiral)
    bool bExpandRadius = false;

    // Radius multiplier at end of life (only used if bExpandRadius is true)
    // 1.0 = same radius, 2.0 = double radius at end, 0.0 = shrink to center
    float RadiusEndMultiplier = 2.0f;

    // ============================================================
    // UParticleModule Overrides
    // ============================================================

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
    virtual int32 GetRequiredBytesPerParticle() const override { return sizeof(FSpiralPayload); }
};
