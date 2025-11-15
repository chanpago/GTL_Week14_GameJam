#pragma once
#include "Object.h"

class USkeletalMeshComponent;
class UAnimSequenceBase;

class UAnimNotifyState : public UObject
{
	//Notify Being
public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration);
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration);
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration);

};