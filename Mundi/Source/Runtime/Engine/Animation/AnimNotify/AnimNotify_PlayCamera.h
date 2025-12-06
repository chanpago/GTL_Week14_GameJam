#pragma once
#include "AnimNotify.h"

class UAnimNotify_PlayCamera : public UAnimNotify
{
public:
	DECLARE_CLASS(UAnimNotify_PlayCamera, UAnimNotify)

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};