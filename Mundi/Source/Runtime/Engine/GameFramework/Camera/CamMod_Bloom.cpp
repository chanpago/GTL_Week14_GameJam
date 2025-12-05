#include "pch.h"
#include "JsonSerializer.h"
#include "CamMod_Bloom.h"

IMPLEMENT_CLASS(UCamMod_Bloom);

void UCamMod_Bloom::CollectPostProcess(TArray<FPostProcessModifier>& Out)
{
	if (!bEnabled) return;

	FPostProcessModifier Modifier;
	Modifier.Type = EPostProcessEffectType::Bloom;
	Modifier.Weight = 1.0f;
	Modifier.Payload.Params0 = FVector4(Threshold, SoftKnee, Intensity, 0.0f);
	Modifier.Payload.Params1 = FVector4(BlurRadius, 0.0f, 0.0f, 0.0f);
}

void UCamMod_Bloom::Serialize(bool bIsLoading, JSON& InOutJson)
{
	if (bIsLoading)
	{
		FJsonSerializer::ReadBool(InOutJson, "bEnabled", bEnabled);
		FJsonSerializer::ReadFloat(InOutJson, "Threshold", Threshold);
		FJsonSerializer::ReadFloat(InOutJson, "SoftKnee", SoftKnee);
		FJsonSerializer::ReadFloat(InOutJson, "Intensity", Intensity);
		FJsonSerializer::ReadFloat(InOutJson, "BlurRadius", BlurRadius);
	}
	else
	{
		InOutJson["bEnabled"] = bEnabled;
		InOutJson["Threshold"] = Threshold;
		InOutJson["SoftKnee"] = SoftKnee;
		InOutJson["Intensity"] = Intensity;
		InOutJson["BlurRadius"] = BlurRadius;
	}
}