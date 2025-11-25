#pragma once
#include "Vector.h" // uint8
#include "ParticleDataContainer.h"
#include "ParticleHelper.h"
#include "Modules/ParticleModuleRequired.h"

struct FDynamicEmitterReplayDataBase
{
    EParticleType EmitterType = EParticleType::Sprite;

    int32 ActiveParticleCount = 0; // Vertex Buffer 크기 계산용
    int32 ParticleStride = 0; // 파티클 하나의 byte stride

    FParticleDataContainer DataContainer;  // ParticleData + ParticleIndices
    FVector Scale = FVector::One();

    virtual ~FDynamicEmitterReplayDataBase() = default;
};

struct FDynamicSpriteEmitterReplayData : public FDynamicEmitterReplayDataBase
{
    UParticleModuleRequired* RequiredModule = nullptr;

    // SubUV 모듈 (있으면 payload에서 SubImageIndex 읽기)
    class UParticleModuleSubUV* SubUVModule = nullptr;
    int32 SubUVPayloadOffset = -1;
};

struct FDynamicMeshEmitterReplayData : public FDynamicEmitterReplayDataBase
{
    UStaticMesh* Mesh = nullptr;
    int32 InstanceStride = 0;
    int32 InstanceCount = 0;
    bool bLighting = false;
};

struct FDynamicEmitterDataBase {
    EParticleType EmitterType = EParticleType::Sprite;
    int32 EmitterIndex = 0; 
    
    bool bUseLocalSpace = false; 
    
    bool bUseSoftParticle = false; 
    float SoftFadeDistance = 50.0f; // 투명 파티클 정렬 기준 
    
    EScreenAlignment Alignment = EScreenAlignment::None; 
    EParticleSortMode SortMode = EParticleSortMode::None; 
    int32 SortPriority = 0; // Emitter 우선순위
    TArray<int32> AsyncSortedIndices;
    
    virtual ~FDynamicEmitterDataBase() = default; 
    
    virtual const FDynamicEmitterReplayDataBase* GetSource() const = 0; 
};

// FDynamicSpriteEmitterDataBase 에서 헷갈려서 아래 네이밍으로 바꿈
// 정렬 속성 상속을 위한 중간 struct
struct FDynamicTranslucentEmitterDataBase : public FDynamicEmitterDataBase
{
    TArray<float> CachedSortKeys;

    virtual const FDynamicEmitterReplayDataBase* GetSource() const = 0;

    void SortParticles(const FVector& ViewOrigin, const FVector& ViewDir, const FMatrix& WorldMatrix, TArray<int32>& OutIndices)
    {
        
        const FDynamicEmitterReplayDataBase* Source = GetSource();
        if (!Source)
        {
            OutIndices.Empty();
            return;
        }

        const int32 NumParticles = Source->ActiveParticleCount;
        if (NumParticles <= 0 || SortMode == EParticleSortMode::None)
        {
            OutIndices.Empty();
            return;
        }

        OutIndices.SetNum(NumParticles);
        for (int32 Index = 0; Index < NumParticles; ++Index)
        {
            OutIndices[Index] = Index;
        }

        if (CachedSortKeys.Num() < NumParticles)
        {
            CachedSortKeys.SetNum(NumParticles);
        }
        
        FVector EffectiveViewOrigin = ViewOrigin;
        FVector EffectiveViewDir = ViewDir;

        if (bUseLocalSpace)
        {
            FMatrix WorldToLocal = WorldMatrix.InverseAffine();
            EffectiveViewOrigin = WorldToLocal.TransformPosition(ViewOrigin);
            EffectiveViewDir = WorldToLocal.TransformVector(ViewDir).GetSafeNormal();
        }

        // 4) 정렬 키 계산
        for (int32 i = 0; i < NumParticles; ++i)
        {
            const FVector Pos = GetParticlePosition(i);

            switch (SortMode)
            {
            case EParticleSortMode::ByDistance:
            {
                const FVector Delta = Pos - EffectiveViewOrigin;
                CachedSortKeys[i] = Delta.SizeSquared();
                break;
            }
            case EParticleSortMode::ByViewDepth:
            {
                const FVector Delta = Pos - EffectiveViewOrigin;
                CachedSortKeys[i] = FVector::Dot(Delta, EffectiveViewDir);
                break;
            }
            case EParticleSortMode::ByAge:
                CachedSortKeys[i] = GetParticleAge(i);
                break;

            default:
                CachedSortKeys[i] = 0.0f;
                break;
            }
        }

        // 5) Back-to-front (큰 키가 먼저)
        OutIndices.Sort([this](int32 A, int32 B)
            {
                return CachedSortKeys[A] > CachedSortKeys[B];
            });
    }

    const FBaseParticle* GetParticle(int32 Idx) const
    {
        const FDynamicEmitterReplayDataBase* Src = GetSource();
        if (!Src || !Src->DataContainer.ParticleData)
            return nullptr;

        const uint8* BasePtr =
            Src->DataContainer.ParticleData +
            Src->ParticleStride * Idx;

        return reinterpret_cast<const FBaseParticle*>(BasePtr);
    }

    FVector GetParticlePosition(int32 Idx) const
    {

        if (const FBaseParticle* P = GetParticle(Idx))
            return P->Location;
        return FVector::Zero();
    }

    float GetParticleAge(int32 Idx) const
    {
        if (const FBaseParticle* P = GetParticle(Idx))
            return P->RelativeTime;
        return 0.0f;
    }
};

struct FDynamicSpriteEmitterData : public FDynamicTranslucentEmitterDataBase
{
    FDynamicSpriteEmitterReplayData Source;

    FDynamicSpriteEmitterData()
    {
        EmitterType = EParticleType::Sprite;
    }

    ~FDynamicSpriteEmitterData() override
    {
        Source.DataContainer.Free();
    }

    const FDynamicEmitterReplayDataBase* GetSource() const override
    {
        return &Source;
    }
};

struct FDynamicMeshEmitterData : public FDynamicTranslucentEmitterDataBase
{
    FDynamicMeshEmitterReplayData Source;

    FDynamicMeshEmitterData()
    {
        EmitterType = EParticleType::Mesh;
    }

    ~FDynamicMeshEmitterData() override
    {
        Source.DataContainer.Free();
    }
    
    const FDynamicEmitterReplayDataBase* GetSource() const override
    {
        return &Source;
    }
};


struct FDynamicBeamEmitterReplayData : public FDynamicEmitterReplayDataBase
{
    UParticleModuleRequired* RequiredModule = nullptr;

    // 빔 세그먼트 데이터
    int32 TessellationFactor = 10;
    float NoiseFrequency = 0.0f;
    float NoiseAmplitude = 0.0f;
};

struct FDynamicBeamEmitterData : public FDynamicEmitterDataBase
{
    FDynamicBeamEmitterReplayData Source;

    FDynamicBeamEmitterData()
    {
        EmitterType = EParticleType::Beam;
    }

    ~FDynamicBeamEmitterData() override
    {
        Source.DataContainer.Free();
    }

    const FDynamicEmitterReplayDataBase* GetSource() const override
    {
        return &Source;
    }
};

