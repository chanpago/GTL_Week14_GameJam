#pragma once
#include "CameraModifierBase.h"
#include "PostProcessing/PostProcessing.h"
#include "UCamMod_Bloom.generated.h"

UCLASS(DisplayName = "Bloom Modifier")
class UCamMod_Bloom : public UCameraModifierBase
{
public:
	GENERATED_REFLECTION_BODY()

	UCamMod_Bloom() = default;
	virtual ~UCamMod_Bloom() = default;

	virtual void ApplyToView(float DeltaTime, FMinimalViewInfo* ViewInfo) override {};

	virtual void CollectPostProcess(TArray<FPostProcessModifier>& Out) override;

	void Serialize(const bool bIsLoading, JSON& InOutHandle) override;

public:
	UPROPERTY(EditAnywhere, Category = "Bloom")
	float Threshold = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bloom")
	float SoftKnee = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Bloom")
	float Intensity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Bloom")
	float BlurRadius = 1.0f;
};

