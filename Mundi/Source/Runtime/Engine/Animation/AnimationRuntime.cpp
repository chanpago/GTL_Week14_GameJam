#include "pch.h"
#include "AnimationRuntime.h"
#include "AnimTypes.h"

ETypeAdvanceAnim FAnimationRuntime::AdvanceTime(const bool bAllowLooping, const float MoveDelta, float& InOutTime, const float EndTime)
{
	float NewTime = InOutTime + MoveDelta;

	if (NewTime < 0.f || NewTime > EndTime)
	{
		if (bAllowLooping)
		{
			if (EndTime != 0.0f)
			{
				NewTime = FMath::Fmod(NewTime, EndTime);
				
				// NewTime이 음수일 경우 (-EndTime, EndTime) 일 수 있으니, +EndTime으로 방어코드 
				if (NewTime < 0.0f)
				{
					NewTime += EndTime;
				}
			}
			// EndTime이 0일 때 
			else
			{
				NewTime = EndTime;
			}

			InOutTime = NewTime;
			return ETAA_Looped;
		}
		else
		{
			InOutTime = FMath::Clamp(NewTime, 0.0f, EndTime);
			return ETAA_Finished;
		}

	}
	
	InOutTime = NewTime;
	return ETAA_Default;
}