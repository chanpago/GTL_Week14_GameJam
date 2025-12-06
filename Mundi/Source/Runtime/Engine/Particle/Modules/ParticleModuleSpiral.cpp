#include "pch.h"
#include "ParticleModuleSpiral.h"
#include "../ParticleEmitter.h"
#include "../ParticleHelper.h"
#include "Source/Runtime/Engine/Particle/ParticleEmitterInstance.h"
#include <cmath>

IMPLEMENT_CLASS(UParticleModuleSpiral)

UParticleModuleSpiral::UParticleModuleSpiral()
{
    bSpawnModule = true;
    bUpdateModule = true;
}

void UParticleModuleSpiral::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
    if (!ParticleBase)
        return;

    // Access payload
    uint8* ParticleBytes = reinterpret_cast<uint8*>(ParticleBase);
    FSpiralPayload* Payload = reinterpret_cast<FSpiralPayload*>(ParticleBytes + Offset);

    // Store the spawn location as the center of orbit
    Payload->CenterPoint = ParticleBase->Location;

    // Set starting angle (convert degrees to radians)
    float StartAngleDegrees = StartAngle.GetValue(Owner->GetRandomFloat());
    Payload->CurrentAngle = DegreesToRadians(StartAngleDegrees);

    // Determine direction
    switch (Direction)
    {
    case ESpiralDirection::Clockwise:
        Payload->DirectionMult = -1.0f;
        break;
    case ESpiralDirection::CounterClockwise:
        Payload->DirectionMult = 1.0f;
        break;
    case ESpiralDirection::Random:
        Payload->DirectionMult = (Owner->GetRandomFloat() > 0.5f) ? 1.0f : -1.0f;
        break;
    }

    // Get initial radius
    float InitialRadius = Radius.GetValue(Owner->GetRandomFloat());

    // Normalize the spiral axis
    FVector Axis = SpiralAxis;
    Axis.Normalize();

    // Calculate two perpendicular vectors to the axis for the orbit plane
    FVector Perp1, Perp2;
    if (FMath::Abs(Axis.Z) < 0.999f)
    {
        Perp1 = FVector::Cross(FVector(0, 0, 1), Axis);
    }
    else
    {
        Perp1 = FVector::Cross(FVector(1, 0, 0), Axis);
    }
    Perp1.Normalize();
    Perp2 = FVector::Cross(Axis, Perp1);
    Perp2.Normalize();

    // Set initial position on the circle
    float CosAngle = std::cos(Payload->CurrentAngle);
    float SinAngle = std::sin(Payload->CurrentAngle);

    FVector Offset2D = Perp1 * CosAngle + Perp2 * SinAngle;
    ParticleBase->Location = Payload->CenterPoint + Offset2D * InitialRadius;
}

void UParticleModuleSpiral::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
    // Normalize the spiral axis once
    FVector Axis = SpiralAxis;
    Axis.Normalize();

    // Calculate perpendicular vectors for the orbit plane
    FVector Perp1, Perp2;
    if (FMath::Abs(Axis.Z) < 0.999f)
    {
        Perp1 = FVector::Cross(FVector(0, 0, 1), Axis);
    }
    else
    {
        Perp1 = FVector::Cross(FVector(1, 0, 0), Axis);
    }
    Perp1.Normalize();
    Perp2 = FVector::Cross(Axis, Perp1);
    Perp2.Normalize();

    BEGIN_UPDATE_LOOP
    {
        // Access payload
        FSpiralPayload& Payload = PARTICLE_ELEMENT(FSpiralPayload, Offset);

        // Move the center point with velocity (so spiral works WITH velocity, not against it)
        Payload.CenterPoint += Particle.Velocity * DeltaTime;

        // Get angular velocity (convert degrees/sec to radians/sec)
        // Use a hash for consistent random per-particle
        float RandomSeed = FloatHash(static_cast<uint32>(reinterpret_cast<uintptr_t>(&Particle)));
        float AngularVel = DegreesToRadians(AngularVelocity.GetValue(RandomSeed));

        // Update angle based on direction and delta time
        Payload.CurrentAngle += AngularVel * Payload.DirectionMult * DeltaTime;

        // Calculate current radius
        float CurrentRadius = Radius.GetValue(RandomSeed);

        // Apply radius expansion if enabled
        if (bExpandRadius)
        {
            float LifeAlpha = Particle.RelativeTime;  // 0 at spawn, 1 at death
            float RadiusMult = FMath::Lerp(1.0f, RadiusEndMultiplier, LifeAlpha);
            CurrentRadius *= RadiusMult;
        }

        // Calculate new position on the circle
        float CosAngle = std::cos(Payload.CurrentAngle);
        float SinAngle = std::sin(Payload.CurrentAngle);

        FVector Offset2D = Perp1 * CosAngle + Perp2 * SinAngle;

        // Store old location for velocity-based rendering
        Particle.OldLocation = Particle.Location;

        // Update particle position (center moves with velocity, spiral orbits around it)
        Particle.Location = Payload.CenterPoint + Offset2D * CurrentRadius;
    }
    END_UPDATE_LOOP;
}
