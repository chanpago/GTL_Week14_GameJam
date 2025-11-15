#include "pch.h"
#include "AnimDateModel.h"
//#include "Math/MathUtility.h"

IMPLEMENT_CLASS(UAnimDataModel)

int32 UAnimDataModel::AddBoneTrack(const FName& BoneName)
{
    // Check if track already exists
    int32 ExistingIndex = FindBoneTrackIndex(BoneName);
    if (ExistingIndex != INDEX_NONE)
    {
        return ExistingIndex;
    }

    // Create new track
    FBoneAnimationTrack NewTrack;
    NewTrack.Name = BoneName;

    int32 NewIndex = BoneAnimationTracks.Add(NewTrack);
    return NewIndex;
}

bool UAnimDataModel::RemoveBoneTrack(const FName& BoneName)
{
    int32 TrackIndex = FindBoneTrackIndex(BoneName);
    if (TrackIndex == INDEX_NONE)
    {
        return false;
    }

    BoneAnimationTracks.RemoveAt(TrackIndex);
    return true;
}

int32 UAnimDataModel::FindBoneTrackIndex(const FName& BoneName) const
{
    for (int32 i = 0; i < BoneAnimationTracks.Num(); ++i)
    {
        if (BoneAnimationTracks[i].Name == BoneName)
        {
            return i;
        }
    }
    return INDEX_NONE;
}

FBoneAnimationTrack* UAnimDataModel::FindBoneTrack(const FName& BoneName)
{
    int32 Index = FindBoneTrackIndex(BoneName);
    if (Index != INDEX_NONE)
    {
        return &BoneAnimationTracks[Index];
    }
    return nullptr;
}

const FBoneAnimationTrack* UAnimDataModel::FindBoneTrack(const FName& BoneName) const
{
    int32 Index = FindBoneTrackIndex(BoneName);
    if (Index != INDEX_NONE)
    {
        return &BoneAnimationTracks[Index];
    }
    return nullptr;
}

bool UAnimDataModel::SetBoneTrackKeys(const FName& BoneName, const TArray<FVector>& PosKeys, const TArray<FQuat>& RotKeys, const TArray<FVector>& ScaleKeys)
{
    FBoneAnimationTrack* Track = FindBoneTrack(BoneName);
    if (!Track)
    {
        return false;
    }

    Track->InternalTrack.PosKeys = PosKeys;
    Track->InternalTrack.RotKeys = RotKeys;
    Track->InternalTrack.ScaleKeys = ScaleKeys;

    return true;
}

bool UAnimDataModel::GetBoneTrackTransform(const FName& BoneName, int32 KeyIndex, FTransform& OutTransform) const
{
    const FBoneAnimationTrack* Track = FindBoneTrack(BoneName);
    if (!Track)
    {
        return false;
    }

    const FRawAnimSequenceTrack& RawTrack = Track->InternalTrack;

    if (KeyIndex < 0)
    {
        return false;
    }

    FVector Position ;
    FQuat Rotation = FQuat::Identity();
    FVector Scale = FVector(1.0f,1.0f,1.0f);

    if (KeyIndex < RawTrack.PosKeys.Num())
    {
        Position = RawTrack.PosKeys[KeyIndex];
    }
    else if (RawTrack.PosKeys.Num() > 0)
    {
        Position = RawTrack.PosKeys[RawTrack.PosKeys.Num() - 1];
    }

    if (KeyIndex < RawTrack.RotKeys.Num())
    {
        Rotation = RawTrack.RotKeys[KeyIndex];
    }
    else if (RawTrack.RotKeys.Num() > 0)
    {
        Rotation = RawTrack.RotKeys[RawTrack.RotKeys.Num() - 1];
    }

    if (KeyIndex < RawTrack.ScaleKeys.Num())
    {
        Scale = RawTrack.ScaleKeys[KeyIndex];
    }
    else if (RawTrack.ScaleKeys.Num() > 0)
    {
        Scale = RawTrack.ScaleKeys[RawTrack.ScaleKeys.Num() - 1];
    }
  
    OutTransform = FTransform(Position, Rotation, Scale);
    return true;
}

FTransform UAnimDataModel::EvaluateBoneTrackTransform(const FName& BoneName, float Time, bool bInterpolate) const
{
    const FBoneAnimationTrack* Track = FindBoneTrack(BoneName);
    if (!Track)
    {
        return FTransform();
    }

    if (NumberOfFrames <= 0 || FrameRate <= 0)
    {
        return FTransform();
    }

    // Convert time to frame
    float FrameTime = Time * static_cast<float>(FrameRate);
    int32 Frame0 = static_cast<int32>(FMath::Floor(FrameTime));
    int32 Frame1 = Frame0 + 1;
    float Alpha = FrameTime - static_cast<float>(Frame0);

    // Clamp frames
    Frame0 = FMath::Clamp(Frame0, 0, NumberOfFrames - 1);
    Frame1 = FMath::Clamp(Frame1, 0, NumberOfFrames - 1);

    FTransform Transform0, Transform1;
    GetBoneTrackTransform(BoneName, Frame0, Transform0);
    GetBoneTrackTransform(BoneName, Frame1, Transform1);

    if (!bInterpolate || FMath::IsNearlyZero(Alpha))
    {
        return Transform0;
    }

    // Interpolate
    FTransform Result;
    Result.Blend(Transform0, Transform1, Alpha);
    return Result;
}