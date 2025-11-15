#pragma once
#include "AnimNotify.h"
#include "Source/Runtime/Engine/Audio/Sound.h"

class UAnimNotify_PlaySound : public UAnimNotify
{
public:
	DECLARE_CLASS(UAnimNotify_PlaySound, UAnimNotify)

	USound* Sound;
	 
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

};
