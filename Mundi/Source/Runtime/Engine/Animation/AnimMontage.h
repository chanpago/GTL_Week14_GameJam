#pragma once
#include "Object.h"
#include "AnimTypes.h"

class UAnimSequence;
class UAnimNotify;
class UAnimNotifyState;

/**
 * @brief 애니메이션 몽타주
 * - 원본 UAnimSequence를 참조하면서 독립적인 노티파이 관리
 * - 상태머신과 별개로 C++ 코드에서 직접 재생
 *
 * @example
 * // 몽타주 생성 및 설정
 * UAnimMontage* HitMontage = NewObject<UAnimMontage>();
 * HitMontage->SetSourceSequence(HitAnimSequence);
 * HitMontage->AddNotify(0.3f, PlaySoundNotify);
 *
 * // 재생
 * AnimInstance->Montage_Play(HitMontage, 0.1f, 0.2f);
 */
class UAnimMontage : public UObject
{
    DECLARE_CLASS(UAnimMontage, UObject)

public:
    UAnimMontage();
    virtual ~UAnimMontage() = default;

    // ============================================================
    // Source Sequence
    // ============================================================

    /**
     * @brief 원본 애니메이션 시퀀스 설정
     * @param InSequence 참조할 원본 시퀀스
     */
    void SetSourceSequence(UAnimSequence* InSequence);

    /**
     * @brief 원본 애니메이션 시퀀스 반환
     */
    UAnimSequence* GetSourceSequence() const { return SourceSequence; }

    /**
     * @brief 재생 길이 반환 (원본 시퀀스 기준)
     */
    float GetPlayLength() const;

    // ============================================================
    // Notify Management
    // ============================================================

    /**
     * @brief 싱글샷 노티파이 추가
     * @param TriggerTime 노티파이 발동 시간 (초)
     * @param Notify 노티파이 객체
     * @param NotifyName 노티파이 이름 (옵션)
     */
    void AddNotify(float TriggerTime, UAnimNotify* Notify, FName NotifyName = FName());

    /**
     * @brief 상태 노티파이 추가 (구간)
     * @param TriggerTime 노티파이 시작 시간 (초)
     * @param Duration 노티파이 지속 시간 (초)
     * @param NotifyState 노티파이 상태 객체
     * @param NotifyName 노티파이 이름 (옵션)
     */
    void AddNotifyState(float TriggerTime, float Duration, UAnimNotifyState* NotifyState, FName NotifyName = FName());

    /**
     * @brief 모든 노티파이 제거
     */
    void ClearNotifies();

    /**
     * @brief 노티파이 배열 반환
     */
    const TArray<FAnimNotifyEvent>& GetNotifies() const { return MontageNotifies; }

    /**
     * @brief 노티파이 배열 반환 (수정 가능)
     */
    TArray<FAnimNotifyEvent>& GetNotifies() { return MontageNotifies; }

    /**
     * @brief 시간 범위 내의 노티파이 수집
     * @param StartTime 시작 시간
     * @param DeltaTime 경과 시간
     * @param OutNotifies 출력 노티파이 배열
     */
    void GetAnimNotifiesInRange(float StartTime, float DeltaTime, TArray<FPendingAnimNotify>& OutNotifies) const;

protected:
    /** 참조할 원본 애니메이션 시퀀스 (포즈 데이터 소스) */
    UAnimSequence* SourceSequence = nullptr;

    /** 몽타주 전용 노티파이 배열 (원본과 독립) */
    TArray<FAnimNotifyEvent> MontageNotifies;
};
