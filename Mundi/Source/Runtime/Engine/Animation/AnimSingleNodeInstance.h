#pragma once
#include "AnimInstance.h"

/**
 * @brief 단일 애니메이션 재생을 위한 AnimInstance
 * - UAnimInstance를 상속받아 하나의 애니메이션만 재생
 * - 상태머신 없이 간단하게 애니메이션을 재생할 때 사용
 * - SkeletalMeshComponent의 AnimationSingleNode 모드에서 자동 생성됨
 *
 * @example 사용 예시:
 *
 * // 방법 1: SkeletalMeshComponent의 PlayAnimation 사용 (권장)
 * UAnimSequence* WalkAnim = RESOURCE.Get<UAnimSequence>("Data/Walk.fbx_Walk");
 * SkeletalMeshComponent->PlayAnimation(WalkAnim, true);
 * // 내부적으로 UAnimSingleNodeInstance가 자동 생성되어 재생
 *
 * // 방법 2: 직접 생성 및 사용
 * UAnimSingleNodeInstance* SingleNodeInstance = NewObject<UAnimSingleNodeInstance>();
 * SkeletalMeshComponent->SetAnimInstance(SingleNodeInstance);
 *
 * UAnimSequence* IdleAnim = RESOURCE.Get<UAnimSequence>("Data/Idle.fbx_Idle");
 * SingleNodeInstance->PlaySingleNode(IdleAnim, true, 1.0f);
 *
 * // 다른 애니메이션으로 전환
 * UAnimSequence* RunAnim = RESOURCE.Get<UAnimSequence>("Data/Run.fbx_Run");
 * SingleNodeInstance->PlaySingleNode(RunAnim, true, 1.5f);
 */
class UAnimSingleNodeInstance : public UAnimInstance
{
    DECLARE_CLASS(UAnimSingleNodeInstance, UAnimInstance)

public:
    UAnimSingleNodeInstance() = default;
    virtual ~UAnimSingleNodeInstance() = default;

    void PlaySingleNode(UAnimSequence* Sequence, bool bLoop = true, float InPlayRate = 1.0f);
    void StopSingleNode();

    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    /**
     * @brief 현재 재생 중인 단일 노드 시퀀스 반환
     */
    UAnimSequence* GetSingleNodeSequence() const { return SingleNodeSequence; }

protected:
    UAnimSequence* SingleNodeSequence = nullptr;
};
