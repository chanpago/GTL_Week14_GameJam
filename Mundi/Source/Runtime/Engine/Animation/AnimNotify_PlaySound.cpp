#include "pch.h"
#include "AnimNotify_PlaySound.h"
#include "SkeletalMeshComponent.h"
#include "AnimSequenceBase.h" 
#include "Source/Runtime/Engine/GameFramework/FAudioDevice.h"

IMPLEMENT_CLASS(UAnimNotify_PlaySound)
void UAnimNotify_PlaySound::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	// Sound 할당하는 임시 코드

	UE_LOG("PlaySound_Notify");

	if (Sound && MeshComp)
	{
		// Sound 재생 
		AActor* Owner = MeshComp->GetOwner();

		FVector SoundPos = MeshComp->GetWorldLocation();

		FAudioDevice::PlaySoundAtLocationOneShot(Sound, SoundPos);
	}

}
