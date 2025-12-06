#pragma once
#include "pch.h"
#include "AnimNotify/AnimNotify.h"
#include "AnimNotify/AnimNotifyState.h"


enum class EAnimationMode : uint8
{
	AnimationBlueprint,
	AnimationSingleNode,
	Custom
};

enum class EPendingNotifyType : uint8
{
	Trigger,
	StateBegin,
	StateTick,
	StateEnd
};

enum ETypeAdvanceAnim : int
{
    ETAA_Default,
    ETAA_Finished,
    ETAA_Looped,
};

struct FAnimNotifyEvent
{
    float GetEndTriggerTime() const { return TriggerTime + Duration; }
    float GetTriggerTime() const { return TriggerTime; }

    bool IsState() const { return  (Duration > 0.0f) && (NotifyState); }
    bool IsSingleShot() const { return (Duration <= 0.0f) && (Notify);  }
    
    UAnimNotify* Notify = nullptr;
    UAnimNotifyState* NotifyState = nullptr;

    float Duration = 0.0f;
    float TriggerTime = 0.0f;
    FName NotifyName;
};

struct FPendingAnimNotify
{
	const FAnimNotifyEvent* Event;
    EPendingNotifyType Type = EPendingNotifyType::Trigger; 
};

enum class EAnimLayer
{
    Base = 0,   // 하체
    Upper = 1,  // 상체
    Count
};

/**
 * @brief 애니메이션 포즈를 제공하는 인터페이스
 * - UAnimSequence, UBlendSpace1D 등이 이 인터페이스를 구현
 * - 모든 애니메이션 소스가 동일한 방식으로 포즈를 출력할 수 있게 함
 */
class IAnimPoseProvider
{
public:
    virtual ~IAnimPoseProvider() = default;

    /**
     * @brief 현재 시간에 해당하는 포즈를 평가
     * @param Time 현재 재생 시간
     * @param DeltaTime 프레임 델타 시간
     * @param OutPose 출력 포즈 (본별 트랜스폼 배열)
     */
    virtual void EvaluatePose(float Time, float DeltaTime, TArray<FTransform>& OutPose) = 0;

    /**
     * @brief 애니메이션 총 재생 길이 반환
     */
    virtual float GetPlayLength() const = 0;

    /**
     * @brief 본 트랙 개수 반환
     */
    virtual int32 GetNumBoneTracks() const = 0;

    /**
     * @brief 노티파이 처리에 사용할 지배적 AnimSequence 반환
     * @return 가장 높은 가중치를 가진 AnimSequence (BlendSpace의 경우)
     *         또는 자기 자신 (AnimSequence의 경우)
     *         노티파이를 지원하지 않으면 nullptr
     */
    virtual class UAnimSequence* GetDominantSequence() const { return nullptr; }

    /**
     * @brief 현재 내부 재생 시간 반환 (노티파이 처리용)
     * @return 현재 재생 시간
     */
    virtual float GetCurrentPlayTime() const { return 0.0f; }

    /**
     * @brief 이전 프레임의 재생 시간 반환 (노티파이 처리용)
     * @return 이전 재생 시간
     */
    virtual float GetPreviousPlayTime() const { return 0.0f; }
};
