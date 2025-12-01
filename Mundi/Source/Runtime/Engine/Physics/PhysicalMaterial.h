#pragma once

// 언리얼 스타일 우선순위: Average(0) < Min(1) < Multiply(2) < Max(3)
enum class ECombineMode : uint8
{
    Average = 0,
    Min = 1,
    Multiply = 2,
    Max = 3
};

class UPhysicalMaterial : public UObject
{
    DECLARE_CLASS(UPhysicalMaterial, UObject)
public:
    float StaticFriction;   // 물체 정지 상태일 시, 움직임 시작하는 걸 방해하는 마찰 계수
    float DynamicFriction;  // 물체 운동 상태일 시, 움직임을 지속하는 걸 방해하는 마찰 계수
    float Restitution;      // 탄성, 0 : 완전 비탄성 1 : 완전 탄성
    float Density;          // TODO : 안 쓰임!! BodySetup::Mass = BodySetup::AggGeom * Density

    // Combine Mode 설정 (기본값: Multiply)
    ECombineMode FrictionCombineMode;
    ECombineMode RestitutionCombineMode;

    UPhysicalMaterial()
        : StaticFriction(1.0f)
        , DynamicFriction(0.5f)
        , Restitution(0.0f)
        , Density(1.0f)
        , FrictionCombineMode(ECombineMode::Multiply)
        , RestitutionCombineMode(ECombineMode::Multiply)
    {
	}

    // 두 Material의 Combine Mode 중 우선순위가 높은 것 반환 (언리얼 스타일)
    static ECombineMode GetPriorityCombineMode(ECombineMode A, ECombineMode B)
    {
        return static_cast<ECombineMode>(FMath::Max(static_cast<uint8>(A), static_cast<uint8>(B)));
    }

    void Serialize(const bool bInIsLoading, JSON& InOutHandle)
    {
        Super::Serialize(bInIsLoading, InOutHandle);

        if (bInIsLoading)
        {
            FJsonSerializer::ReadFloat(InOutHandle, "StaticFriction", StaticFriction);
            FJsonSerializer::ReadFloat(InOutHandle, "DynamicFriction", DynamicFriction);
            FJsonSerializer::ReadFloat(InOutHandle, "Restitution", Restitution);
            FJsonSerializer::ReadFloat(InOutHandle, "Density", Density);

            int32 FrictionMode = static_cast<int32>(FrictionCombineMode);
            int32 RestitutionMode = static_cast<int32>(RestitutionCombineMode);
            FJsonSerializer::ReadInt32(InOutHandle, "FrictionCombineMode", FrictionMode);
            FJsonSerializer::ReadInt32(InOutHandle, "RestitutionCombineMode", RestitutionMode);
            FrictionCombineMode = static_cast<ECombineMode>(FrictionMode);
            RestitutionCombineMode = static_cast<ECombineMode>(RestitutionMode);
        }
        else
        {
            InOutHandle["StaticFriction"] = StaticFriction;
            InOutHandle["DynamicFriction"] = DynamicFriction;
            InOutHandle["Restitution"] = Restitution;
            InOutHandle["Density"] = Density;
            InOutHandle["FrictionCombineMode"] = static_cast<int32>(FrictionCombineMode);
            InOutHandle["RestitutionCombineMode"] = static_cast<int32>(RestitutionCombineMode);
        }
    }
};
