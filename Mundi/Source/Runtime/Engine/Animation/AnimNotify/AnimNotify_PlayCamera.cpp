#include "pch.h"
#include "AnimNotify_PlayCamera.h"

#include "Source/Runtime/Engine/GameFramework/PlayerCameraManager.h"
#include "Source/Runtime/Engine/GameFramework/World.h"
#include "Source/Runtime/Engine/Components/SkeletalMeshComponent.h"


IMPLEMENT_CLASS(UAnimNotify_PlayCamera)

void UAnimNotify_PlayCamera::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp)
    {
        return;
    }

    UWorld* World = MeshComp->GetWorld();
    APlayerCameraManager* CameraManager = World ? World->GetPlayerCameraManager() : nullptr;
    if (!CameraManager)
    {
        return;
    }

    switch (EffectType)
    {
    case ECameraNotifyEffect::Shake:
        CameraManager->StartCameraShake(
            ShakeSettings.Duration,
            ShakeSettings.AmplitudeLocation,
            ShakeSettings.AmplitudeRotationDeg,
            ShakeSettings.Frequency,
            ShakeSettings.Priority);
        break;
    case ECameraNotifyEffect::Fade:
        CameraManager->StartFade(
            FadeSettings.Duration,
            FadeSettings.FromAlpha,
            FadeSettings.ToAlpha,
            FadeSettings.Color,
            FadeSettings.Priority);
        break;
    case ECameraNotifyEffect::LetterBox:
        CameraManager->StartLetterBox(
            LetterBoxSettings.Duration,
            LetterBoxSettings.Aspect,
            LetterBoxSettings.BarHeight,
            LetterBoxSettings.Color,
            LetterBoxSettings.Priority);
        break;
    case ECameraNotifyEffect::Vignette:
        CameraManager->StartVignette(
            VignetteSettings.Duration,
            VignetteSettings.Radius,
            VignetteSettings.Softness,
            VignetteSettings.Intensity,
            VignetteSettings.Roundness,
            VignetteSettings.Color,
            VignetteSettings.Priority);
        break;
    case ECameraNotifyEffect::Gamma:
        CameraManager->StartGamma(GammaSettings.Gamma);
        break;
    case ECameraNotifyEffect::DOF:
        CameraManager->StartDOF(
            DOFSettings.FocalDistance,
            DOFSettings.FocalRegion,
            DOFSettings.NearTransition,
            DOFSettings.FarTransition,
            DOFSettings.MaxNearBlur,
            DOFSettings.MaxFarBlur,
            DOFSettings.Priority);
        break;
    default:
        break;
    }
}
