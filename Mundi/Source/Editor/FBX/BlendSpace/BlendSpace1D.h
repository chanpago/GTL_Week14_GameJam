#pragma once
#include "Object.h"

class UAnimSequence;

/**
 * @brief 1D Blend Space Sample Point
 */
struct FBlendSample1D
{
	UAnimSequence* Animation = nullptr; // 블렌딩할 애니메이션
	float Position = 0.0f;  // Position on parameter axis (e.g., Speed value)

	FBlendSample1D() = default;
	FBlendSample1D(UAnimSequence* InAnimation, float InPosition)
		: Animation(InAnimation), Position(InPosition) {}
};

/**
 * @brief 1D Blend Space
 * - Linear interpolation of multiple animations based on single parameter (e.g., Speed)
 *
 * @example Usage
 *
 * // 1. Create BlendSpace1D
 * UBlendSpace1D* LocomotionBS = NewObject<UBlendSpace1D>();
 *
 * // 2. Add samples
 * LocomotionBS->AddSample(IdleAnim, 0.0f);    // Speed=0 -> Idle
 * LocomotionBS->AddSample(WalkAnim, 100.0f);  // Speed=100 -> Walk
 * LocomotionBS->AddSample(RunAnim, 200.0f);   // Speed=200 -> Run
 *
 * // 3. Update every frame
 * float CurrentSpeed = Character->GetVelocity().Size();
 * TArray<FTransform> OutputPose;
 * LocomotionBS->Update(CurrentSpeed, DeltaTime, OutputPose);
 */
class UBlendSpace1D : public UObject
{
	DECLARE_CLASS(UBlendSpace1D, UObject)

public:
	UBlendSpace1D() = default;
	virtual ~UBlendSpace1D() = default;

	// ============================================================
	// Setup
	// ============================================================

	/**
	 * @brief Add sample
	 * @param Animation Animation to blend
	 * @param Position Position on parameter axis
	 */
	void AddSample(UAnimSequence* Animation, float Position);

	/**
	 * @brief Remove all samples
	 */
	void ClearSamples();

	/**
	 * @brief Get number of samples
	 */
	int32 GetNumSamples() const { return Samples.Num(); }

	/**
	 * @brief Get all samples (for editor visualization)
	 */
	const TArray<FBlendSample1D>& GetSamples() const { return Samples; }

	/**
	 * @brief Get sample at index
	 */
	const FBlendSample1D* GetSample(int32 Index) const;

	/**
	 * @brief Set sample position
	 * @param Index Sample index
	 * @param NewPosition New position value
	 */
	bool SetSamplePosition(int32 Index, float NewPosition);

	/**
	 * @brief Set sample animation
	 * @param Index Sample index
	 * @param Animation New animation
	 */
	bool SetSampleAnimation(int32 Index, UAnimSequence* Animation);

	/**
	 * @brief Remove sample at index
	 */
	bool RemoveSample(int32 Index);

	// ============================================================
	// Parameter Range
	// ============================================================

	/**
	 * @brief Set parameter range (for clamping and visualization)
	 */
	void SetParameterRange(float InMin, float InMax);

	/**
	 * @brief Get minimum parameter value
	 */
	float GetMinParameter() const { return MinParameter; }

	/**
	 * @brief Get maximum parameter value
	 */
	float GetMaxParameter() const { return MaxParameter; }

	// ============================================================
	// Evaluation
	// ============================================================

	/**
	 * @brief Calculate blended pose based on parameter value
	 * @param Parameter Current parameter value (e.g., Speed)
	 * @param DeltaTime Frame delta time
	 * @param OutPose Output pose
	 */
	void Update(float Parameter, float DeltaTime, TArray<FTransform>& OutPose);

	/**
	 * @brief Get current parameter value
	 */
	float GetParameter() const { return CurrentParameter; }

	/**
	 * @brief Reset play time
	 */
	void ResetPlayTime() { CurrentPlayTime = 0.0f; }

private:
	// Sample points (sorted by Position)
	TArray<FBlendSample1D> Samples;

	// Current parameter value
	float CurrentParameter = 0.0f;

	// Current play time (all samples synchronized)
	float CurrentPlayTime = 0.0f;

	// Parameter range
	float MinParameter = 0.0f;
	float MaxParameter = 200.0f;

	// ============================================================
	// Internal Helpers
	// ============================================================

	void SortSamples();

	void GetBlendSamplesAndWeight(float Parameter,
		FBlendSample1D*& OutSampleA,
		FBlendSample1D*& OutSampleB,
		float& OutAlpha);

	void EvaluateAnimation(UAnimSequence* Animation, float Time, TArray<FTransform>& OutPose);

	void BlendPoses(const TArray<FTransform>& PoseA,
		const TArray<FTransform>& PoseB,
		float Alpha,
		TArray<FTransform>& OutPose);
};
