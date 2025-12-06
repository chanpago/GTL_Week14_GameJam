#include "pch.h"
#include "AnimNotifyState.h"

void UAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{ 
	// 기본적으로는 아무것도 안함. 파생 클래스에서 override해서 호출 
	UE_LOG("Notify Begin");
}

void UAnimNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	// 기본적으로는 아무것도 안함. 파생 클래스에서 override해서 호출 
	UE_LOG("Notify Tick");
}

void UAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	// 기본적으로는 아무것도 안함. 파생 클래스에서 override해서 호출 
	UE_LOG("Notify End");
}
