#pragma once
#include "ParticleModuleTypeDataBase.h"

class UParticleModuleBeam : public UParticleModuleTypeDataBase
{
    DECLARE_CLASS(UParticleModuleBeam, UParticleModuleTypeDataBase)
public:
    UParticleModuleBeam();

    // 빔 설정
    FVector SourcePoint = FVector::Zero();  // 시작점
    FVector TargetPoint = FVector(100, 0, 0);  // 끝점

    // 소스/타겟이 액터를 따라가는 경우
    AActor* SourceActor = nullptr;
    AActor* TargetActor = nullptr;

    // 빔 세그먼트 개수 (부드러운 곡선을 위해)
    int32 TessellationFactor = 10;

    // 빔 노이즈 설정 (번개 효과 등)
    float NoiseFrequency = 0.0f;
    float NoiseAmplitude = 0.0f;

    // 타입별 파티클 추가 데이터 크기
    virtual int32 GetRequiredParticleBytes() const override
    {
        return sizeof(FVector) * 2;  // SourcePoint + TargetPoint
    }

    virtual int32 GetDynamicVertexStride() const override
    {
        // Position + UV + Color
        return sizeof(FVector) + sizeof(FVector2D) + sizeof(FLinearColor);
    }
};
