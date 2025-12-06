#include "pch.h"
#include "CamMod_Bloom.h"

IMPLEMENT_CLASS(UCamMod_Bloom);

void UCamMod_Bloom::Serialize(const bool bIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bIsLoading, InOutHandle);
}

void UCamMod_Bloom::CollectPostProcess(TArray<FPostProcessModifier>& Out)
{
    if (!bEnabled)
    {
        return;
    }

    FPostProcessModifier Modifier;
    Modifier.Type = EPostProcessEffectType::Bloom;
    Modifier.Priority = Priority;
    Modifier.bEnabled = true;
    Modifier.Weight = Weight;
    Modifier.SourceObject = this;

    Modifier.Payload.Params0 = FVector4(Threshold, SoftKnee, Intensity, 0.0f);
    Modifier.Payload.Params1 = FVector4(BlurRadius, 0.0f, 0.0f, 0.0f);

    Out.Add(Modifier);
}