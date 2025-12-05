#pragma once
#include "CameraModifierBase.h"
#include "PostProcessing/PostProcessing.h"

class UCamMod_Bloom : public UCameraModifierBase
{
public:
	DECLARE_CLASS(UCamMod_Bloom, UCameraModifierBase)

	UCamMod_Bloom() = default;
	virtual ~UCamMod_Bloom() = default;

	virtual void ApplyToView(float DeltaTime, FMinimalViewInfo* ViewInfo) override {};

	virtual void CollectPostProcess(TArray<FPostProcessModifier>& Out) override;

public:
	float Threshold = 1.0f;
	float SoftKnee = 0.5f;
	float Intensity = 1.0f;
	float BlurRadius = 1.0f;
};

