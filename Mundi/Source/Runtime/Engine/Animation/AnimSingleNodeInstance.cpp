#include "pch.h"
#include "AnimSingleNodeInstance.h"
#include "AnimSequence.h"

void UAnimSingleNodeInstance::PlaySingleNode(UAnimSequence* Sequence, bool bLoop, float InPlayRate)
{
    if (!Sequence)
    {
        UE_LOG("UAnimSingleNodeInstance::PlaySingleNode - Invalid sequence");
        StopSingleNode();
        return;
    }

    SingleNodeSequence = Sequence;
    PlaySequence(Sequence, bLoop, InPlayRate);
}

void UAnimSingleNodeInstance::StopSingleNode()
{
    SingleNodeSequence = nullptr;
    StopSequence();
}

void UAnimSingleNodeInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    if (!SingleNodeSequence)
    {
        return;
    }

    UAnimInstance::NativeUpdateAnimation(DeltaSeconds);
}

