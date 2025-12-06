#include "pch.h"
#include "AnimNotify.h"
#include "AnimSequenceBase.h"

IMPLEMENT_CLASS(UAnimNotify)

void UAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	// 기본적으로는 아무것도 안함. 파생 클래스에서 override해서 호출  
	UE_LOG("Notify");
}
