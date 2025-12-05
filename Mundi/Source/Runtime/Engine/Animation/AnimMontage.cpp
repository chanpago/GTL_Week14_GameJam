#include "pch.h"
#include "AnimMontage.h"
#include "AnimSequence.h"
#include "AnimNotify.h"
#include "AnimNotifyState.h"

IMPLEMENT_CLASS(UAnimMontage)

UAnimMontage::UAnimMontage()
    : SourceSequence(nullptr)
{
}

// ============================================================
// Source Sequence
// ============================================================

void UAnimMontage::SetSourceSequence(UAnimSequence* InSequence)
{
    SourceSequence = InSequence;

    if (SourceSequence)
    {
        UE_LOG("UAnimMontage::SetSourceSequence - Set source: %s",
            SourceSequence->ObjectName.ToString().c_str());
    }
}

float UAnimMontage::GetPlayLength() const
{
    if (SourceSequence)
    {
        return SourceSequence->GetPlayLength();
    }
    return 0.0f;
}

// ============================================================
// Notify Management
// ============================================================

void UAnimMontage::AddNotify(float TriggerTime, UAnimNotify* Notify, FName NotifyName)
{
    if (!Notify)
    {
        UE_LOG("UAnimMontage::AddNotify - Invalid notify");
        return;
    }

    FAnimNotifyEvent Event;
    Event.TriggerTime = TriggerTime;
    Event.Duration = 0.0f;  // 싱글샷
    Event.Notify = Notify;
    Event.NotifyState = nullptr;
    Event.NotifyName = NotifyName;

    MontageNotifies.Add(Event);

    UE_LOG("UAnimMontage::AddNotify - Added notify '%s' at %.2f",
        NotifyName.ToString().c_str(), TriggerTime);
}

void UAnimMontage::AddNotifyState(float TriggerTime, float Duration, UAnimNotifyState* NotifyState, FName NotifyName)
{
    if (!NotifyState)
    {
        UE_LOG("UAnimMontage::AddNotifyState - Invalid notify state");
        return;
    }

    FAnimNotifyEvent Event;
    Event.TriggerTime = TriggerTime;
    Event.Duration = Duration;
    Event.Notify = nullptr;
    Event.NotifyState = NotifyState;
    Event.NotifyName = NotifyName;

    MontageNotifies.Add(Event);

    UE_LOG("UAnimMontage::AddNotifyState - Added notify state '%s' at %.2f (Duration: %.2f)",
        NotifyName.ToString().c_str(), TriggerTime, Duration);
}

void UAnimMontage::ClearNotifies()
{
    MontageNotifies.Empty();
    UE_LOG("UAnimMontage::ClearNotifies - Cleared all notifies");
}

void UAnimMontage::GetAnimNotifiesInRange(float StartTime, float DeltaTime, TArray<FPendingAnimNotify>& OutNotifies) const
{
    OutNotifies.Empty();

    if (MontageNotifies.Num() == 0 || DeltaTime <= 0.0f)
    {
        return;
    }

    const float MinTime = StartTime;
    const float MaxTime = StartTime + DeltaTime;

    for (int32 i = 0; i < MontageNotifies.Num(); ++i)
    {
        const FAnimNotifyEvent& Event = MontageNotifies[i];

        // 싱글샷 노티파이
        if (Event.IsSingleShot())
        {
            const float NotifyTime = Event.GetTriggerTime();

            // 범위 내에 있는지 확인 (MinTime < NotifyTime <= MaxTime)
            if (MinTime < NotifyTime && NotifyTime <= MaxTime)
            {
                FPendingAnimNotify Pending;
                Pending.Event = &Event;
                Pending.Type = EPendingNotifyType::Trigger;
                OutNotifies.Add(Pending);
            }
        }
        // 상태 노티파이 (구간)
        else if (Event.IsState())
        {
            const float NotifyStart = Event.GetTriggerTime();
            const float NotifyEnd = Event.GetEndTriggerTime();

            // Begin: 시작 시간이 범위 내
            if (MinTime < NotifyStart && NotifyStart <= MaxTime)
            {
                FPendingAnimNotify Pending;
                Pending.Event = &Event;
                Pending.Type = EPendingNotifyType::StateBegin;
                OutNotifies.Add(Pending);
            }

            // Tick: 범위가 노티파이 구간과 겹침
            if (MaxTime > NotifyStart && MinTime < NotifyEnd)
            {
                FPendingAnimNotify Pending;
                Pending.Event = &Event;
                Pending.Type = EPendingNotifyType::StateTick;
                OutNotifies.Add(Pending);
            }

            // End: 종료 시간이 범위 내
            if (MinTime < NotifyEnd && NotifyEnd <= MaxTime)
            {
                FPendingAnimNotify Pending;
                Pending.Event = &Event;
                Pending.Type = EPendingNotifyType::StateEnd;
                OutNotifies.Add(Pending);
            }
        }
    }
}
