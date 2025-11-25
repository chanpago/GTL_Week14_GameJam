#include "pch.h"
#include "ParticleModuleColor.h"
#include "../ParticleEmitter.h"
#include "../ParticleHelper.h"
#include "Source/Runtime/Engine/Particle/ParticleEmitterInstance.h"

IMPLEMENT_CLASS(UParticleModuleColor)

UParticleModuleColor::UParticleModuleColor()
{
    bSpawnModule = true;
    bUpdateModule = true;
}

void UParticleModuleColor::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
    if (!ParticleBase)
        return;

    float ColorRatio = Owner->GetRandomFloat(); 
    float AlphaRatio = Owner->GetRandomFloat();
    
    // 초기 색상 설정
    ParticleBase->Color = StartColor.GetValue(ColorRatio);
    ParticleBase->Color.A = StartAlpha.GetValue(AlphaRatio);

    // Base 색상도 저장 (나중에 참조용)
    ParticleBase->BaseColor = ParticleBase->Color;
}

void UParticleModuleColor::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
    // BEGIN_UPDATE_LOOP 매크로 사용
    BEGIN_UPDATE_LOOP
    {
        if (bUseColorOverLife)
        {
            FLinearColor NewColor = ColorOverLife.GetValue(Particle.RelativeTime);
            Particle.Color.R = NewColor.R;
            Particle.Color.G = NewColor.G;
            Particle.Color.B = NewColor.B;
        }

        if (bUseAlphaOverLife)
        {
            Particle.Color.A = AlphaOverLife.GetValue(Particle.RelativeTime);
        }
    }
    END_UPDATE_LOOP;
}
