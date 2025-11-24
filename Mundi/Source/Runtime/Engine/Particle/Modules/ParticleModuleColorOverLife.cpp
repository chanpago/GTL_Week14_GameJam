#include "pch.h"
#include "ParticleModuleColorOverLife.h"
#include "../ParticleEmitter.h"
#include "../ParticleHelper.h"

IMPLEMENT_CLASS(UParticleModuleColorOverLife)

UParticleModuleColorOverLife::UParticleModuleColorOverLife()
{
    ModuleType = EParticleModuleType::Update;
    bUpdateModule = true;
}

float UParticleModuleColorOverLife::EvaluateAlphaCurve(float t) const
{
    // t는 RelativeTime (0~1)

    // 점1 이전: Point1Value로 고정
    if (t < AlphaPoint1Time)
    {
        return AlphaPoint1Value;
    }
    // 점2 이후: Point2Value로 고정
    else if (t >= AlphaPoint2Time)
    {
        return AlphaPoint2Value;
    }
    // 점1과 점2 사이: 선형 보간
    else
    {
        float alpha = (t - AlphaPoint1Time) / (AlphaPoint2Time - AlphaPoint1Time);
        return FMath::Lerp(AlphaPoint1Value, AlphaPoint2Value, alpha);
    }
}

void UParticleModuleColorOverLife::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
    // BEGIN_UPDATE_LOOP 매크로 사용 - 모든 활성 파티클 순회
    BEGIN_UPDATE_LOOP
    {
        // RelativeTime은 0~1 값 (Age / Lifetime)
        float t = Particle.RelativeTime;

        // Color(t) 커브에서 값을 가져와 Particle.Color에 적용
        if (bUseColorOverLife)
        {
            FLinearColor NewColor = ColorOverLife.GetValue(t);
            Particle.Color.R = NewColor.R;
            Particle.Color.G = NewColor.G;
            Particle.Color.B = NewColor.B;
        }

        // Alpha(t) 커브에서 값을 가져와 Particle.Color.A에 적용
        if (bUseAlphaOverLife)
        {
            // 2-point 커브 시스템 사용
            float AlphaValue = EvaluateAlphaCurve(t);
            Particle.Color.A = AlphaValue;

            // 첫 번째 파티클만 로그 출력
            static int LogCount = 0;
            if (LogCount < 5)
            {
                UE_LOG("[ColorOverLife] t=%.3f, Alpha=%.3f, Color=(%.2f, %.2f, %.2f, %.2f)",
                    t, AlphaValue, Particle.Color.R, Particle.Color.G, Particle.Color.B, Particle.Color.A);
                LogCount++;
            }
        }
    }
    END_UPDATE_LOOP;
}
