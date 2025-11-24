#include "pch.h"
#include "ParticleModuleRotation.h"
#include "../ParticleEmitterInstance.h"
#include "../ParticleHelper.h"

IMPLEMENT_CLASS(UParticleModuleRotation)

UParticleModuleRotation::UParticleModuleRotation()
{
    ModuleType = EParticleModuleType::Spawn;
    bSpawnModule = true;
}

void UParticleModuleRotation::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
    if (!ParticleBase)
        return;

    // 랜덤 값 생성 (0~1 범위)
    float RandomValue = Owner->GetRandomFloat();

    // 초기 회전각 설정 (라디안 단위)
    float RotationValue = StartRotation.GetValue(RandomValue);

    ParticleBase->Rotation = RotationValue;
    ParticleBase->BaseRotation = RotationValue;

    UE_LOG("ParticleModuleRotation::Spawn - Set rotation to: %f radians", RotationValue);
}
