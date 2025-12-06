#pragma once
#include "AnimNotify.h"

enum class ECameraNotifyEffect : uint8
{
    Shake,
    Fade,
    LetterBox,
    Vignette,
    Gamma,
    DOF
};

struct FCameraShakeSettings
{
    float Duration = 0.35f;
    float AmplitudeLocation = 0.15f;
    float AmplitudeRotationDeg = 0.15f;
    float Frequency = 30.0f;
    int32 Priority = 0;
};

struct FCameraFadeSettings
{
    float Duration = 0.5f;
    float FromAlpha = 0.0f;
    float ToAlpha = 1.0f;
    FLinearColor Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
    int32 Priority = 0;
};

struct FCameraLetterBoxSettings
{
    float Duration = 0.5f;
    float Aspect = 2.35f;
    float BarHeight = 0.1f;
    FLinearColor Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
    int32 Priority = 0;
};

struct FCameraVignetteSettings
{
    float Duration = 0.5f;
    float Radius = 0.7f;
    float Softness = 0.5f;
    float Intensity = 0.7f;
    float Roundness = 1.0f;
    FLinearColor Color = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
    int32 Priority = 0;
};

struct FCameraGammaSettings
{
    float Gamma = 1.0f;
};

struct FCameraDOFSettings
{
    float FocalDistance = 5.0f;
    float FocalRegion = 0.5f;
    float NearTransition = 2.0f;
    float FarTransition = 5.0f;
    float MaxNearBlur = 32.0f;
    float MaxFarBlur = 32.0f;
    int32 Priority = 0;
};

class UAnimNotify_PlayCamera : public UAnimNotify
{
public:
    DECLARE_CLASS(UAnimNotify_PlayCamera, UAnimNotify)

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

    ECameraNotifyEffect EffectType = ECameraNotifyEffect::Shake;
    FCameraShakeSettings ShakeSettings;
    FCameraFadeSettings FadeSettings;
    FCameraLetterBoxSettings LetterBoxSettings;
    FCameraVignetteSettings VignetteSettings;
    FCameraGammaSettings GammaSettings;
    FCameraDOFSettings DOFSettings;
};
