#include "pch.h"
#include "ParticleModuleLocation.h"
#include "../ParticleEmitter.h"
#include "Source/Runtime/Engine/Particle/ParticleEmitterInstance.h"
#include "Source/Runtime/Engine/Particle/ParticleHelper.h"

IMPLEMENT_CLASS(UParticleModuleLocation)

UParticleModuleLocation::UParticleModuleLocation()
{
    bSpawnModule = true;
}

void UParticleModuleLocation::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
    if (!ParticleBase)
        return;

    // 랜덤 시드 생성
    float RandomValue = Owner->GetRandomFloat();

    FVector SpawnLocation = FVector(0.0f,0.0f,0.0f);

    switch (DistributionType)
    {
    case ELocationDistributionType::Point:
        SpawnLocation = StartLocation.GetValue(RandomValue);
        break;

    case ELocationDistributionType::Box:
        {
            // 박스 영역 내 랜덤 위치 - 각 축마다 별도의 랜덤값 사용
            float rx = (Owner->GetRandomFloat() * 2.0f - 1.0f) * BoxExtent.X;
            float ry = (Owner->GetRandomFloat() * 2.0f - 1.0f) * BoxExtent.Y;
            float rz = (Owner->GetRandomFloat() * 2.0f - 1.0f) * BoxExtent.Z;
            SpawnLocation = FVector(rx, ry, rz);
        }
        break;

    case ELocationDistributionType::Sphere:
        {
            // 구 영역 내 랜덤 위치 - 각 파라미터마다 별도의 랜덤값 사용
            float theta = Owner->GetRandomFloat() * 2.0f * PI;
            float phi = Owner->GetRandomFloat() * PI;
            float r = Owner->GetRandomFloat() * SphereRadius;

            SpawnLocation.X = r * sinf(phi) * cosf(theta);
            SpawnLocation.Y = r * sinf(phi) * sinf(theta);
            SpawnLocation.Z = r * cosf(phi);
        }
        break;

    case ELocationDistributionType::Cylinder:
        {
            // 원통 영역 내 랜덤 위치 - 각 파라미터마다 별도의 랜덤값 사용
            float angle = Owner->GetRandomFloat() * 2.0f * PI;
            float radius = Owner->GetRandomFloat() * CylinderRadius;
            float height = (Owner->GetRandomFloat() * 2.0f - 1.0f) * CylinderHeight * 0.5f;

            SpawnLocation.X = radius * cosf(angle);
            SpawnLocation.Y = radius * sinf(angle);
            SpawnLocation.Z = height;
        }
        break;
    }

    ParticleBase->Location = SpawnLocation;
    ParticleBase->OldLocation = SpawnLocation;
}
