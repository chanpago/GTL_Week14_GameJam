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

    const FRawAnimSequenceTrack& RawTrack = Track->InternalTrack;

    // 시간 클램프
    float PlayLength = this->PlayLength;
    Time = FMath::Clamp(Time, 0.0f, PlayLength);

    // 시간을 프레임 번호로 변환
    float FrameTime = Time * static_cast<float>(FrameRate);

    // 프레임 인덱스 계산 (KraftonGTL 방식)
    int32 FrameIndex0 = FMath::FloorToInt(FrameTime);
    int32 FrameIndex1 = FMath::CeilToInt(FrameTime);
    float Alpha = FrameTime - FrameIndex0;

    FTransform Result;

    // Position 보간 (Linear)
    if (RawTrack.PosKeys.Num() > 0)
    {
        int32 PosIdx0 = FMath::Clamp(FrameIndex0, 0, RawTrack.PosKeys.Num() - 1);
        int32 PosIdx1 = FMath::Clamp(FrameIndex1, 0, RawTrack.PosKeys.Num() - 1);

        const FVector& Pos0 = RawTrack.PosKeys[PosIdx0];
        const FVector& Pos1 = RawTrack.PosKeys[PosIdx1];

        Result.Translation = FVector::Lerp(Pos0, Pos1, Alpha);
    }

    // Rotation 보간 (Slerp)
    if (RawTrack.RotKeys.Num() > 0)
    {
        int32 RotIdx0 = FMath::Clamp(FrameIndex0, 0, RawTrack.RotKeys.Num() - 1);
        int32 RotIdx1 = FMath::Clamp(FrameIndex1, 0, RawTrack.RotKeys.Num() - 1);

        const FQuat& Rot0 = RawTrack.RotKeys[RotIdx0];
        const FQuat& Rot1 = RawTrack.RotKeys[RotIdx1];

        Result.Rotation = FQuat::Slerp(Rot0, Rot1, Alpha);
        Result.Rotation.Normalize();
    }

    // Scale 보간 (Linear)
    if (RawTrack.ScaleKeys.Num() > 0)
    {
        int32 ScaleIdx0 = FMath::Clamp(FrameIndex0, 0, RawTrack.ScaleKeys.Num() - 1);
        int32 ScaleIdx1 = FMath::Clamp(FrameIndex1, 0, RawTrack.ScaleKeys.Num() - 1);

        const FVector& Scale0 = RawTrack.ScaleKeys[ScaleIdx0];
        const FVector& Scale1 = RawTrack.ScaleKeys[ScaleIdx1];

        Result.Scale3D = FVector::Lerp(Scale0, Scale1, Alpha);
    }

    return Result;
}