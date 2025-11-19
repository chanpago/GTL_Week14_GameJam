#include "pch.h"
#include "BlendSpace1D.h"
#include "Source/Runtime/Engine/Animation/AnimSequence.h"
#include "Source/Runtime/Engine/Animation/AnimDateModel.h"

IMPLEMENT_CLASS(UBlendSpace1D)

void UBlendSpace1D::AddSample(UAnimSequence* Animation, float Position)
{
	if (!Animation)
	{
		UE_LOG("UBlendSpace1D::AddSample - Invalid animation");
		return;
	}

	Samples.Add(FBlendSample1D(Animation, Position));
	SortSamples();

	UE_LOG("UBlendSpace1D::AddSample - Added '%s' at position %.1f (Total: %d samples)",
		Animation->ObjectName.ToString().c_str(), Position, Samples.Num());
}

void UBlendSpace1D::ClearSamples()
{
	Samples.Empty();
	CurrentPlayTime = 0.0f;
}

const FBlendSample1D* UBlendSpace1D::GetSample(int32 Index) const
{
	if (Index >= 0 && Index < Samples.Num())
	{
		return &Samples[Index];
	}
	return nullptr;
}

bool UBlendSpace1D::SetSamplePosition(int32 Index, float NewPosition)
{
	if (Index < 0 || Index >= Samples.Num())
	{
		return false;
	}

	Samples[Index].Position = NewPosition;
	SortSamples();
	return true;
}

bool UBlendSpace1D::SetSampleAnimation(int32 Index, UAnimSequence* Animation)
{
	if (Index < 0 || Index >= Samples.Num())
	{
		return false;
	}

	if (!Animation)
	{
		return false;
	}

	Samples[Index].Animation = Animation;
	return true;
}

bool UBlendSpace1D::RemoveSample(int32 Index)
{
	if (Index < 0 || Index >= Samples.Num())
	{
		return false;
	}

	Samples.RemoveAt(Index);
	return true;
}

void UBlendSpace1D::SetParameterRange(float InMin, float InMax)
{
	MinParameter = InMin;
	MaxParameter = InMax;

	// Ensure Min <= Max
	if (MinParameter > MaxParameter)
	{
		float Temp = MinParameter;
		MinParameter = MaxParameter;
		MaxParameter = Temp;
	}
}

void UBlendSpace1D::SortSamples()
{
	// Sort by Position in ascending order
	Samples.Sort([](const FBlendSample1D& A, const FBlendSample1D& B)
	{
		return A.Position < B.Position;
	});
}

void UBlendSpace1D::Update(float Parameter, float DeltaTime, TArray<FTransform>& OutPose)
{
	if (Samples.Num() == 0)
	{
		return;
	}

	CurrentParameter = Parameter;

	// If only 1 sample, output without blending
	if (Samples.Num() == 1)
	{
		EvaluateAnimation(Samples[0].Animation, CurrentPlayTime, OutPose);

		// Update play time
		if (Samples[0].Animation)
		{
			float PlayLength = Samples[0].Animation->GetPlayLength();
			CurrentPlayTime += DeltaTime;
			if (PlayLength > 0.0f)
			{
				CurrentPlayTime = fmod(CurrentPlayTime, PlayLength);
			}
		}
		return;
	}

	// Get two samples to blend and weight
	FBlendSample1D* SampleA = nullptr;
	FBlendSample1D* SampleB = nullptr;
	float Alpha = 0.0f;

	GetBlendSamplesAndWeight(Parameter, SampleA, SampleB, Alpha);

	if (!SampleA || !SampleB)
	{
		return;
	}

	// Extract pose from each sample
	TArray<FTransform> PoseA;
	TArray<FTransform> PoseB;

	EvaluateAnimation(SampleA->Animation, CurrentPlayTime, PoseA);
	EvaluateAnimation(SampleB->Animation, CurrentPlayTime, PoseB);

	// Blend two poses
	BlendPoses(PoseA, PoseB, Alpha, OutPose);

	// Update play time (loop based on shorter animation)
	float PlayLengthA = SampleA->Animation ? SampleA->Animation->GetPlayLength() : 0.0f;
	float PlayLengthB = SampleB->Animation ? SampleB->Animation->GetPlayLength() : 0.0f;
	float MinPlayLength = FMath::Min(PlayLengthA, PlayLengthB);

	CurrentPlayTime += DeltaTime;
	if (MinPlayLength > 0.0f)
	{
		CurrentPlayTime = fmod(CurrentPlayTime, MinPlayLength);
	}
}

void UBlendSpace1D::GetBlendSamplesAndWeight(float Parameter,
	FBlendSample1D*& OutSampleA,
	FBlendSample1D*& OutSampleB,
	float& OutAlpha)
{
	// Handle out of range
	if (Parameter <= Samples[0].Position)
	{
		// Below minimum -> first sample only
		OutSampleA = &Samples[0];
		OutSampleB = &Samples[0];
		OutAlpha = 0.0f;
		return;
	}

	if (Parameter >= Samples[Samples.Num() - 1].Position)
	{
		// Above maximum -> last sample only
		OutSampleA = &Samples[Samples.Num() - 1];
		OutSampleB = &Samples[Samples.Num() - 1];
		OutAlpha = 0.0f;
		return;
	}

	// Find the segment containing the parameter
	for (int32 i = 0; i < Samples.Num() - 1; ++i)
	{
		if (Parameter >= Samples[i].Position && Parameter <= Samples[i + 1].Position)
		{
			OutSampleA = &Samples[i];
			OutSampleB = &Samples[i + 1];

			// Calculate linear interpolation alpha
			float Range = Samples[i + 1].Position - Samples[i].Position;
			if (Range > 0.0f)
			{
				OutAlpha = (Parameter - Samples[i].Position) / Range;
			}
			else
			{
				OutAlpha = 0.0f;
			}
			return;
		}
	}

	// Default (should not reach here)
	OutSampleA = &Samples[0];
	OutSampleB = &Samples[0];
	OutAlpha = 0.0f;
}

void UBlendSpace1D::EvaluateAnimation(UAnimSequence* Animation, float Time, TArray<FTransform>& OutPose)
{
	if (!Animation)
	{
		return;
	}

	UAnimDataModel* DataModel = Animation->GetDataModel();
	if (!DataModel)
	{
		return;
	}

	// Get number of bones
	int32 NumBones = DataModel->GetNumBoneTracks();
	OutPose.SetNum(NumBones);

	float PlayLength = Animation->GetPlayLength();
	if (PlayLength <= 0.0f)
	{
		return;
	}

	// Normalize time to 0~PlayLength range
	float NormalizedTime = fmod(Time, PlayLength);
	if (NormalizedTime < 0.0f)
	{
		NormalizedTime += PlayLength;
	}

	// Extract transform for each bone
	const TArray<FBoneAnimationTrack>& BoneTracks = DataModel->GetBoneAnimationTracks();
	for (int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
	{
		if (BoneIndex < BoneTracks.Num())
		{
			OutPose[BoneIndex] = DataModel->EvaluateBoneTrackTransform(BoneTracks[BoneIndex].Name, NormalizedTime);
		}
		else
		{
			OutPose[BoneIndex] = FTransform();
		}
	}
}

void UBlendSpace1D::BlendPoses(const TArray<FTransform>& PoseA,
	const TArray<FTransform>& PoseB,
	float Alpha,
	TArray<FTransform>& OutPose)
{
	int32 NumBones = FMath::Min(PoseA.Num(), PoseB.Num());
	OutPose.SetNum(NumBones);

	for (int32 i = 0; i < NumBones; ++i)
	{
		const FTransform& TransformA = PoseA[i];
		const FTransform& TransformB = PoseB[i];

		// Linear interpolation for position
		FVector BlendedPosition = FVector::Lerp(TransformA.Translation, TransformB.Translation, Alpha);

		// Spherical linear interpolation for rotation (Slerp)
		FQuat BlendedRotation = FQuat::Slerp(TransformA.Rotation, TransformB.Rotation, Alpha);

		// Linear interpolation for scale
		FVector BlendedScale = FVector::Lerp(TransformA.Scale3D, TransformB.Scale3D, Alpha);

		OutPose[i] = FTransform(BlendedPosition, BlendedRotation, BlendedScale);
	}
}
