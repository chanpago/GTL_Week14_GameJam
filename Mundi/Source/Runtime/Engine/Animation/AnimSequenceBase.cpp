#include "pch.h"
#include "AnimSequenceBase.h"
#include "ObjectFactory.h"
#include "AnimSequence.h"
#include "AnimationRuntime.h"
#include "AnimNotify/AnimNotify_PlaySound.h"
#include "AnimNotify/AnimNotify_PlayCamera.h"
#include "AnimNotify/AnimNotify_PlayParticle.h"
#include "AnimNotify/AnimNotify_EnableHitbox.h"
#include "AnimNotify/AnimNotify_EnableWeaponCollision.h"
#include "AnimTypes.h"
#include "JsonSerializer.h"
#include "Source/Runtime/AssetManagement/ResourceManager.h"
#include "Source/Runtime/Engine/Audio/Sound.h"
#include "Source/Runtime/Engine/Particle/ParticleSystem.h"
#include "Source/Runtime/Core/Misc/PathUtils.h"
#include <filesystem>



namespace
{
    FString CameraEffectToString(ECameraNotifyEffect Type)
    {
        switch (Type)
        {
        case ECameraNotifyEffect::Shake: return "Shake";
        case ECameraNotifyEffect::Fade: return "Fade";
        case ECameraNotifyEffect::LetterBox: return "LetterBox";
        case ECameraNotifyEffect::Vignette: return "Vignette";
        case ECameraNotifyEffect::Gamma: return "Gamma";
        case ECameraNotifyEffect::DOF: return "DOF";
        default: return "Shake";
        }
    }

    ECameraNotifyEffect CameraEffectFromString(const FString& Str)
    {
        if (Str == "Fade") return ECameraNotifyEffect::Fade;
        if (Str == "LetterBox") return ECameraNotifyEffect::LetterBox;
        if (Str == "Vignette") return ECameraNotifyEffect::Vignette;
        if (Str == "Gamma") return ECameraNotifyEffect::Gamma;
        if (Str == "DOF") return ECameraNotifyEffect::DOF;
        return ECameraNotifyEffect::Shake;
    }
}
IMPLEMENT_CLASS(UAnimSequenceBase)

UAnimSequenceBase::UAnimSequenceBase()
    : RateScale(1.0f)
    , bLoop(true)
    , DataModel(nullptr)
{
    // Create DataModel 
    DataModel = NewObject<UAnimDataModel>();
}

UAnimDataModel* UAnimSequenceBase::GetDataModel() const
{
    return DataModel;
}

bool UAnimSequenceBase::IsNotifyAvailable() const
{
    if (Notifies.Num() == 0)
    {
        UAnimSequenceBase* Self = const_cast<UAnimSequenceBase*>(this);
        (void)Self->GetAnimNotifyEvents();
    }
    return (Notifies.Num() != 0) && (GetPlayLength() > 0.f);
}

TArray<FAnimNotifyEvent>& UAnimSequenceBase::GetAnimNotifyEvents()
{
    // Lazy-load meta if empty and sidecar exists (only attempt once)
    if (Notifies.Num() == 0 && !bMetaLoadAttempted)
    {
        bMetaLoadAttempted = true;

        FString FileName = "AnimNotifies";
        const FString Src = GetFilePath();
        if (!Src.empty())
        {
            size_t pos = Src.find_last_of("/\\");
            FString Just = (pos == FString::npos) ? Src : Src.substr(pos + 1);
            size_t dot = Just.find_last_of('.');
            if (dot != FString::npos) Just = Just.substr(0, dot);
            if (!Just.empty()) FileName = Just;
        }
        const FString MetaPathUtf8 = NormalizePath(GDataDir + "/" + FileName + ".anim.json");
        std::filesystem::path MetaPath(UTF8ToWide(MetaPathUtf8));
        std::error_code ec;

        if (std::filesystem::exists(MetaPath, ec))
        {
            LoadMeta(MetaPathUtf8);
            UE_LOG("GetAnimNotifyEvents - Loaded %d notifies from %s", Notifies.Num(), MetaPathUtf8.c_str());
        }
    }
    return Notifies;
}

const TArray<FAnimNotifyEvent>& UAnimSequenceBase::GetAnimNotifyEvents() const
{
    if (Notifies.Num() == 0)
    {
        // const-correctness workaround
        UAnimSequenceBase* Self = const_cast<UAnimSequenceBase*>(this);
        (void)Self->GetAnimNotifyEvents();
    }
    return Notifies;
}

void UAnimSequenceBase::GetAnimNotify(const float& StartTime, const float& DeltaTime, TArray<FPendingAnimNotify>& OutNotifies) const
{
    OutNotifies.Empty();

    if (!IsNotifyAvailable())
    {
        return;
    }

    bool const bPlayinfBackywards = (DeltaTime < 0.f);
    float PreviousPosition = StartTime;
    float CurrentPosition = StartTime;
    float DesiredDeltaMove = DeltaTime;
    const float PlayLength = GetPlayLength();

    // 한 틱 동안 애니메이션 여러번 돌게 될 때,
    // notify가 호출되는 상한선을 정해주는 함수
    uint32_t MaxLoopCount = 2;
    if (PlayLength > 0.0f && FMath::Abs(DeltaTime) > PlayLength)
    {
        MaxLoopCount = FMath::Clamp((DesiredDeltaMove / PlayLength), 2.0f, 1000.0f); 
    }

    // 단일 패스로 처리 (루프 시 두 구간으로 나눔)
    float EndPosition = StartTime + DeltaTime;

    if (EndPosition > PlayLength && bLoop)
    {
        // 애니메이션이 루프하는 경우
        // 구간 1: StartTime → PlayLength
        GetAnimNotifiesFromDeltaPosition(StartTime, PlayLength, OutNotifies);

        // 남은 시간 계산 (루프 후 위치)
        float Remainder = FMath::Fmod(EndPosition, PlayLength);

        // 구간 2: 0 → Remainder
        if (Remainder > 0.0f)
        {
            GetAnimNotifiesFromDeltaPosition(0.0f, Remainder, OutNotifies);
        }
    }
    else if (EndPosition <= PlayLength)
    {
        // 일반적인 경우: 루프 없음
        GetAnimNotifiesFromDeltaPosition(StartTime, EndPosition, OutNotifies);
    }
    else
    {
        // 루프하지 않는 애니메이션이 끝에 도달
        GetAnimNotifiesFromDeltaPosition(StartTime, PlayLength, OutNotifies);
    }
     
}

void UAnimSequenceBase::GetAnimNotifiesFromDeltaPosition(const float& PreviousPosition, const float& CurrentPosition, TArray<FPendingAnimNotify>& OutNotifies) const
{
    if (Notifies.Num() == 0)
    {
        return;
    }

    bool const bPlayingBackwards = (CurrentPosition < PreviousPosition);

    const float MinTime = bPlayingBackwards ? CurrentPosition : PreviousPosition;
    const float MaxTime = bPlayingBackwards ? PreviousPosition : CurrentPosition;
     
    for (int32 NotifyIndex = 0; NotifyIndex < Notifies.Num(); ++NotifyIndex)
    {
        const FAnimNotifyEvent& AnimNotifyEvent = Notifies[NotifyIndex];

        const float NotifyStartTime = AnimNotifyEvent.GetTriggerTime();
        const float NotifyEndTime = AnimNotifyEvent.GetEndTriggerTime();

        const bool bIsState = AnimNotifyEvent.IsState();
        const bool bIsSingleShot = AnimNotifyEvent.IsSingleShot();

        if (!bPlayingBackwards)
        {
            // 정방향 
            if (bIsSingleShot)
            {
                if (MinTime < NotifyStartTime && MaxTime >= NotifyStartTime)
                {
                    FPendingAnimNotify Pending;
                    Pending.Event = &AnimNotifyEvent;
                    Pending.Type = EPendingNotifyType::Trigger;

                    OutNotifies.Add(Pending);
                    
                }
            }
            else if (bIsState)
            {
                const bool bNotIntersects = (MinTime >= NotifyEndTime || MaxTime < NotifyStartTime);

                // 교집합이 없으면 건너뜀
                if (bNotIntersects)
                {
                    continue;
                }

                // State중 Begin 판별
                if (NotifyStartTime >= MinTime && NotifyStartTime < MaxTime)
                {
                    FPendingAnimNotify PendingBegin;
                    PendingBegin.Event = &AnimNotifyEvent;
                    PendingBegin.Type = EPendingNotifyType::StateBegin;
                    OutNotifies.Add(PendingBegin);
                }

                // State중 Tick 판별 
                {
                    FPendingAnimNotify PendingTick;
                    PendingTick.Event = &AnimNotifyEvent;
                    PendingTick.Type = EPendingNotifyType::StateTick;
                    OutNotifies.Add(PendingTick);
                }

                // State중 End 판별 
                if (NotifyEndTime > MinTime && NotifyEndTime <= MaxTime)
                {
                    FPendingAnimNotify PendingEnd;
                    PendingEnd.Event = &AnimNotifyEvent;
                    PendingEnd.Type = EPendingNotifyType::StateEnd;
                    OutNotifies.Add(PendingEnd);
                }
            } 
        } 
        // 역방향 
        else
        {
            if (bIsSingleShot)
            {
                //if (MinTime < NotifyStartTime && MaxTime >= NotifyStartTime) 
                if (MinTime >=  NotifyEndTime && MaxTime < NotifyStartTime)
                {
                    FPendingAnimNotify Pending;
                    Pending.Event = &AnimNotifyEvent;
                    Pending.Type = EPendingNotifyType::Trigger;

                    OutNotifies.Add(Pending);
                }
            }
            // TODO: 역방향 Notify는 정상작동하는 지 아직 체크 못해봄
            else if (bIsState)
            {
                const bool bNotIntersects = (MaxTime < NotifyStartTime || MinTime >= NotifyEndTime);
                if (bNotIntersects)
                {
                    continue;
                }

                if (MaxTime >= NotifyEndTime && MinTime < NotifyEndTime)
                {
                    FPendingAnimNotify PendingEnd;
                    PendingEnd.Event = &AnimNotifyEvent;
                    PendingEnd.Type = EPendingNotifyType::StateEnd;
                    OutNotifies.Add(PendingEnd);
                }

                {
                    FPendingAnimNotify PendingTick;
                    PendingTick.Event = &AnimNotifyEvent;
                    PendingTick.Type = EPendingNotifyType::StateTick;
                    OutNotifies.Add(PendingTick);
                }

                if (NotifyStartTime < MaxTime && NotifyStartTime >= MinTime)
                {
                    FPendingAnimNotify PendingBegin;
                    PendingBegin.Event = &AnimNotifyEvent;
                    PendingBegin.Type = EPendingNotifyType::StateBegin;
                    OutNotifies.Add(PendingBegin);
                }

            }
           
        }
    }
}

void UAnimSequenceBase::AddPlaySoundNotify(float Time, UAnimNotify* Notify, float Duration)
{
    if (!Notify)
    {
        return;
    }

    FAnimNotifyEvent NewEvent;
    NewEvent.TriggerTime = Time;
    NewEvent.Duration = Duration;
    NewEvent.Notify = Notify;
    NewEvent.NotifyState = nullptr;

    Notifies.Add(NewEvent);
}

void UAnimSequenceBase::AddPlayParticleNotify(float Time, UAnimNotify* Notify, float Duration)
{
    AddPlaySoundNotify(Time, Notify, Duration);
}

void UAnimSequenceBase::AddPlayCameraNotify(float Time, UAnimNotify* Notify, float Duration)
{
    AddPlaySoundNotify(Time, Notify, Duration);
}

bool UAnimSequenceBase::SaveMeta(const FString& MetaPathUTF8) const
{
    if (MetaPathUTF8.empty())
    {
        return false;
    }
     

    JSON Root = JSON::Make(JSON::Class::Object);
    Root["Type"] = "AnimSequenceMeta";
    Root["Version"] = 1;

    JSON NotifyArray = JSON::Make(JSON::Class::Array);
    for (const FAnimNotifyEvent& Evt : Notifies)
    {
        JSON Item = JSON::Make(JSON::Class::Object);
        Item["TriggerTime"] = Evt.TriggerTime;
        Item["Duration"] = Evt.Duration;
        Item["NotifyName"] = Evt.NotifyName.ToString().c_str();

        if (Evt.IsSingleShot())
        {
            Item["Kind"] = "Single";
        }
        else if (Evt.IsState())
        {
            Item["Kind"] = "State";
        }
        else
        {
            Item["Kind"] = "Unknown";
        }

        FString ClassName;
        if (Evt.Notify)
        {
            ClassName = Evt.Notify->GetClass()->Name; // e.g., UAnimNotify_PlaySound
        }
        else if (Evt.NotifyState)
        {
            ClassName = Evt.NotifyState->GetClass()->Name;
        }
        else
        {
            ClassName = "";
        }
        Item["Class"] = ClassName.c_str();

        JSON Data = JSON::Make(JSON::Class::Object);
        // Class-specific data
        if (Evt.Notify && Evt.Notify->IsA<UAnimNotify_PlaySound>())
        {
            const UAnimNotify_PlaySound* PS = static_cast<const UAnimNotify_PlaySound*>(Evt.Notify);
            FString Path = (PS && PS->Sound) ? PS->Sound->GetFilePath() : "";
            Data["SoundPath"] = Path.c_str();
        }
        else if (Evt.Notify && Evt.Notify->IsA<UAnimNotify_PlayParticle>())
        {
            const UAnimNotify_PlayParticle* ParticleNotify = static_cast<const UAnimNotify_PlayParticle*>(Evt.Notify);
            FString ParticlePath = (ParticleNotify && ParticleNotify->ParticleSystem) ? ParticleNotify->ParticleSystem->GetFilePath() : "";
            Data["ParticlePath"] = ParticlePath.c_str();
            Data["LocationOffset"] = FJsonSerializer::VectorToJson(ParticleNotify->LocationOffset);
            Data["RotationOffset"] = FJsonSerializer::VectorToJson(ParticleNotify->RotationOffset);
            Data["ScaleOffset"] = FJsonSerializer::VectorToJson(ParticleNotify->ScaleOffset);
            Data["AttachBone"] = ParticleNotify->AttachBoneName.ToString().c_str();
            Data["AttachToOwner"] = ParticleNotify->bAttachToOwner;
            Data["LifeTime"] = ParticleNotify->LifeTime;
        }

        else if (Evt.Notify && Evt.Notify->IsA<UAnimNotify_EnableHitbox>())
        {
            const UAnimNotify_EnableHitbox* HB = static_cast<const UAnimNotify_EnableHitbox*>(Evt.Notify);
            Data["Damage"] = HB->Damage;
            Data["DamageType"] = HB->DamageType.c_str();
            Data["HitboxExtent"] = FJsonSerializer::VectorToJson(HB->HitboxExtent);
            Data["HitboxOffset"] = FJsonSerializer::VectorToJson(HB->HitboxOffset);
            Data["Duration"] = HB->Duration;
        }
        else if (Evt.Notify && Evt.Notify->IsA<UAnimNotify_EnableWeaponCollision>())
        {
            const UAnimNotify_EnableWeaponCollision* WC = static_cast<const UAnimNotify_EnableWeaponCollision*>(Evt.Notify);
            Data["bEnable"] = WC->bEnable;
        }
        else if (Evt.Notify && Evt.Notify->IsA<UAnimNotify_PlayCamera>())
        {
            const UAnimNotify_PlayCamera* CameraNotify = static_cast<const UAnimNotify_PlayCamera*>(Evt.Notify);
            JSON CameraData = JSON::Make(JSON::Class::Object);
            CameraData["Type"] = CameraEffectToString(CameraNotify->EffectType).c_str();
            switch (CameraNotify->EffectType)
            {
            case ECameraNotifyEffect::Shake:
                CameraData["Duration"] = CameraNotify->ShakeSettings.Duration;
                CameraData["AmpLoc"] = CameraNotify->ShakeSettings.AmplitudeLocation;
                CameraData["AmpRot"] = CameraNotify->ShakeSettings.AmplitudeRotationDeg;
                CameraData["Frequency"] = CameraNotify->ShakeSettings.Frequency;
                CameraData["Priority"] = CameraNotify->ShakeSettings.Priority;
                break;
            case ECameraNotifyEffect::Fade:
                CameraData["Duration"] = CameraNotify->FadeSettings.Duration;
                CameraData["FromAlpha"] = CameraNotify->FadeSettings.FromAlpha;
                CameraData["ToAlpha"] = CameraNotify->FadeSettings.ToAlpha;
                CameraData["Color"] = FJsonSerializer::Vector4ToJson(CameraNotify->FadeSettings.Color.ToFVector4());
                CameraData["Priority"] = CameraNotify->FadeSettings.Priority;
                break;
            case ECameraNotifyEffect::LetterBox:
                CameraData["Duration"] = CameraNotify->LetterBoxSettings.Duration;
                CameraData["Aspect"] = CameraNotify->LetterBoxSettings.Aspect;
                CameraData["BarHeight"] = CameraNotify->LetterBoxSettings.BarHeight;
                CameraData["Color"] = FJsonSerializer::Vector4ToJson(CameraNotify->LetterBoxSettings.Color.ToFVector4());
                CameraData["Priority"] = CameraNotify->LetterBoxSettings.Priority;
                break;
            case ECameraNotifyEffect::Vignette:
                CameraData["Duration"] = CameraNotify->VignetteSettings.Duration;
                CameraData["Radius"] = CameraNotify->VignetteSettings.Radius;
                CameraData["Softness"] = CameraNotify->VignetteSettings.Softness;
                CameraData["Intensity"] = CameraNotify->VignetteSettings.Intensity;
                CameraData["Roundness"] = CameraNotify->VignetteSettings.Roundness;
                CameraData["Color"] = FJsonSerializer::Vector4ToJson(CameraNotify->VignetteSettings.Color.ToFVector4());
                CameraData["Priority"] = CameraNotify->VignetteSettings.Priority;
                break;
            case ECameraNotifyEffect::Gamma:
                CameraData["Gamma"] = CameraNotify->GammaSettings.Gamma;
                break;
            case ECameraNotifyEffect::DOF:
                CameraData["FocalDistance"] = CameraNotify->DOFSettings.FocalDistance;
                CameraData["FocalRegion"] = CameraNotify->DOFSettings.FocalRegion;
                CameraData["NearTransition"] = CameraNotify->DOFSettings.NearTransition;
                CameraData["FarTransition"] = CameraNotify->DOFSettings.FarTransition;
                CameraData["MaxNearBlur"] = CameraNotify->DOFSettings.MaxNearBlur;
                CameraData["MaxFarBlur"] = CameraNotify->DOFSettings.MaxFarBlur;
                CameraData["Priority"] = CameraNotify->DOFSettings.Priority;
                break;
            default:
                break;
            }

            Data["Camera"] = CameraData;
        }
        Item["Data"] = Data;

        NotifyArray.append(Item);
    }

    Root["Notifies"] = NotifyArray;

    FWideString WPath = UTF8ToWide(MetaPathUTF8);
    return FJsonSerializer::SaveJsonToFile(Root, WPath);
}

bool UAnimSequenceBase::LoadMeta(const FString& MetaPathUTF8)
{
    if (MetaPathUTF8.empty())
    {
        return false;
    }

    JSON Root;
    FWideString WPath = UTF8ToWide(MetaPathUTF8);
    if (!FJsonSerializer::LoadJsonFromFile(Root, WPath))
    {
        return false;
    }

    if (!Root.hasKey("Notifies"))
    {
        return false;
    }

    const JSON& Arr = Root.at("Notifies");
    if (Arr.JSONType() != JSON::Class::Array)
    {
        return false;
    }

    Notifies.Empty();

    for (size_t i = 0; i < Arr.size(); ++i)
    {
        const JSON& Item = Arr.at(i);
        if (Item.JSONType() != JSON::Class::Object)
        {
            continue;
        }

        FAnimNotifyEvent Evt;

        // TriggerTime, Duration, NotifyName
        float TriggerTime = 0.0f;
        float Duration = 0.0f;
        FJsonSerializer::ReadFloat(Item, "TriggerTime", TriggerTime, 0.0f, false);
        FJsonSerializer::ReadFloat(Item, "Duration", Duration, 0.0f, false);
        FString NameStr;
        if (Item.hasKey("NotifyName"))
        {
            NameStr = Item.at("NotifyName").ToString();
        }
        Evt.TriggerTime = TriggerTime;
        Evt.Duration = Duration;
        Evt.NotifyName = FName(NameStr);

        // Class-specific reconstruction
        FString ClassStr;
        if (Item.hasKey("Class"))
        {
            ClassStr = Item.at("Class").ToString();
        }
        const JSON* DataPtr = nullptr;
        if (Item.hasKey("Data"))
        {
            DataPtr = &Item.at("Data");
        }

        if (ClassStr == "UAnimNotify_PlaySound" || ClassStr == "PlaySound")
        {
            UAnimNotify_PlaySound* PS = NewObject<UAnimNotify_PlaySound>();
            if (PS && DataPtr)
            {
                if (DataPtr->hasKey("SoundPath"))
                {
                    FString SP = DataPtr->at("SoundPath").ToString();
                    if (!SP.empty())
                    {
                        USound* Loaded = UResourceManager::GetInstance().Load<USound>(SP);
                        PS->Sound = Loaded;
                    }
                }
            }
            Evt.Notify = PS;
            Evt.NotifyState = nullptr;
        }
        else if (ClassStr == "UAnimNotify_PlayParticle" || ClassStr == "PlayParticle")
        {
            UAnimNotify_PlayParticle* ParticleNotify = NewObject<UAnimNotify_PlayParticle>();
            if (ParticleNotify && DataPtr)
            {
                if (DataPtr->hasKey("ParticlePath"))
                {
                    FString ParticlePath = DataPtr->at("ParticlePath").ToString();
                    if (!ParticlePath.empty())
                    {
                        UParticleSystem* LoadedSystem = UResourceManager::GetInstance().Load<UParticleSystem>(ParticlePath);
                        ParticleNotify->ParticleSystem = LoadedSystem;
                    }
                }

                FJsonSerializer::ReadVector(*DataPtr, "LocationOffset", ParticleNotify->LocationOffset, ParticleNotify->LocationOffset, false);
                FJsonSerializer::ReadVector(*DataPtr, "RotationOffset", ParticleNotify->RotationOffset, ParticleNotify->RotationOffset, false);
                FJsonSerializer::ReadVector(*DataPtr, "ScaleOffset", ParticleNotify->ScaleOffset, ParticleNotify->ScaleOffset, false);
                FJsonSerializer::ReadBool(*DataPtr, "AttachToOwner", ParticleNotify->bAttachToOwner, ParticleNotify->bAttachToOwner, false);
                FJsonSerializer::ReadFloat(*DataPtr, "LifeTime", ParticleNotify->LifeTime, ParticleNotify->LifeTime, false);

                if (DataPtr->hasKey("AttachBone"))
                {
                    FString BoneName = DataPtr->at("AttachBone").ToString();
                    if (!BoneName.empty())
                    {
                        ParticleNotify->AttachBoneName = FName(BoneName);
                    }
                }
            }

            if (ParticleNotify)
            {
                ParticleNotify->SetEventDuration(Duration);
            }

            Evt.Notify = ParticleNotify;
            Evt.NotifyState = nullptr;
        }
        else if (ClassStr == "UAnimNotify_EnableHitbox" || ClassStr == "EnableHitbox")
        {
            UAnimNotify_EnableHitbox* HB = NewObject<UAnimNotify_EnableHitbox>();
            if (HB && DataPtr)
            {
                FJsonSerializer::ReadFloat(*DataPtr, "Damage", HB->Damage, HB->Damage, false);
                if (DataPtr->hasKey("DamageType"))
                {
                    HB->DamageType = DataPtr->at("DamageType").ToString();
                }
                FJsonSerializer::ReadVector(*DataPtr, "HitboxExtent", HB->HitboxExtent, HB->HitboxExtent, false);
                FJsonSerializer::ReadVector(*DataPtr, "HitboxOffset", HB->HitboxOffset, HB->HitboxOffset, false);
                FJsonSerializer::ReadFloat(*DataPtr, "Duration", HB->Duration, HB->Duration, false);
            }
            Evt.Notify = HB;
            Evt.NotifyState = nullptr;
        }
        else if (ClassStr == "UAnimNotify_EnableWeaponCollision" || ClassStr == "EnableWeaponCollision")
        {
            UAnimNotify_EnableWeaponCollision* WC = NewObject<UAnimNotify_EnableWeaponCollision>();
            if (WC && DataPtr)
            {
                if (DataPtr->hasKey("bEnable"))
                {
                    WC->bEnable = DataPtr->at("bEnable").ToBool();
                }
            }
            Evt.Notify = WC;
            Evt.NotifyState = nullptr;
        }
        else if (ClassStr == "UAnimNotify_PlayCamera" || ClassStr == "PlayCamera")
        {
            UAnimNotify_PlayCamera* CameraNotify = NewObject<UAnimNotify_PlayCamera>();
            if (CameraNotify && DataPtr)
            {
                const JSON* CameraJson = DataPtr;
                if (CameraJson->hasKey("Camera"))
                {
                    CameraJson = &CameraJson->at("Camera");
                }

                if (CameraJson)
                {
                    FString TypeStr = CameraJson->hasKey("Type") ? CameraJson->at("Type").ToString() : "Shake";
                    CameraNotify->EffectType = CameraEffectFromString(TypeStr);

                    FVector4 ColorVec(0, 0, 0, 1);
                    switch (CameraNotify->EffectType)
                    {
                    case ECameraNotifyEffect::Shake:
                        FJsonSerializer::ReadFloat(*CameraJson, "Duration", CameraNotify->ShakeSettings.Duration, CameraNotify->ShakeSettings.Duration, false);
                        FJsonSerializer::ReadFloat(*CameraJson, "AmpLoc", CameraNotify->ShakeSettings.AmplitudeLocation, CameraNotify->ShakeSettings.AmplitudeLocation, false);
                        FJsonSerializer::ReadFloat(*CameraJson, "AmpRot", CameraNotify->ShakeSettings.AmplitudeRotationDeg, CameraNotify->ShakeSettings.AmplitudeRotationDeg, false);
                        FJsonSerializer::ReadFloat(*CameraJson, "Frequency", CameraNotify->ShakeSettings.Frequency, CameraNotify->ShakeSettings.Frequency, false);
                        { float TempPriority = static_cast<float>(CameraNotify->ShakeSettings.Priority); FJsonSerializer::ReadFloat(*CameraJson, "Priority", TempPriority, TempPriority, false); CameraNotify->ShakeSettings.Priority = static_cast<int32>(TempPriority); }
                        break;
                    case ECameraNotifyEffect::Fade:
                        FJsonSerializer::ReadFloat(*CameraJson, "Duration", CameraNotify->FadeSettings.Duration, CameraNotify->FadeSettings.Duration, false);
                        FJsonSerializer::ReadFloat(*CameraJson, "FromAlpha", CameraNotify->FadeSettings.FromAlpha, CameraNotify->FadeSettings.FromAlpha, false);
                        FJsonSerializer::ReadFloat(*CameraJson, "ToAlpha", CameraNotify->FadeSettings.ToAlpha, CameraNotify->FadeSettings.ToAlpha, false);
                        if (FJsonSerializer::ReadVector4(*CameraJson, "Color", ColorVec, CameraNotify->FadeSettings.Color.ToFVector4(), false))
                        {
                            CameraNotify->FadeSettings.Color = FLinearColor(ColorVec.X, ColorVec.Y, ColorVec.Z, ColorVec.W);
                        }
                        { float TempPriority = static_cast<float>(CameraNotify->FadeSettings.Priority); FJsonSerializer::ReadFloat(*CameraJson, "Priority", TempPriority, TempPriority, false); CameraNotify->FadeSettings.Priority = static_cast<int32>(TempPriority); }
                        break;
                    case ECameraNotifyEffect::LetterBox:
                        FJsonSerializer::ReadFloat(*CameraJson, "Duration", CameraNotify->LetterBoxSettings.Duration, CameraNotify->LetterBoxSettings.Duration, false);
                        FJsonSerializer::ReadFloat(*CameraJson, "Aspect", CameraNotify->LetterBoxSettings.Aspect, CameraNotify->LetterBoxSettings.Aspect, false);
                        FJsonSerializer::ReadFloat(*CameraJson, "BarHeight", CameraNotify->LetterBoxSettings.BarHeight, CameraNotify->LetterBoxSettings.BarHeight, false);
                        if (FJsonSerializer::ReadVector4(*CameraJson, "Color", ColorVec, CameraNotify->LetterBoxSettings.Color.ToFVector4(), false))
                        {
                            CameraNotify->LetterBoxSettings.Color = FLinearColor(ColorVec.X, ColorVec.Y, ColorVec.Z, ColorVec.W);
                        }
                        { float TempPriority = static_cast<float>(CameraNotify->LetterBoxSettings.Priority); FJsonSerializer::ReadFloat(*CameraJson, "Priority", TempPriority, TempPriority, false); CameraNotify->LetterBoxSettings.Priority = static_cast<int32>(TempPriority); }
                        break;
                    case ECameraNotifyEffect::Vignette:
                        FJsonSerializer::ReadFloat(*CameraJson, "Duration", CameraNotify->VignetteSettings.Duration, CameraNotify->VignetteSettings.Duration, false);
                        FJsonSerializer::ReadFloat(*CameraJson, "Radius", CameraNotify->VignetteSettings.Radius, CameraNotify->VignetteSettings.Radius, false);
                        FJsonSerializer::ReadFloat(*CameraJson, "Softness", CameraNotify->VignetteSettings.Softness, CameraNotify->VignetteSettings.Softness, false);
                        FJsonSerializer::ReadFloat(*CameraJson, "Intensity", CameraNotify->VignetteSettings.Intensity, CameraNotify->VignetteSettings.Intensity, false);
                        FJsonSerializer::ReadFloat(*CameraJson, "Roundness", CameraNotify->VignetteSettings.Roundness, CameraNotify->VignetteSettings.Roundness, false);
                        if (FJsonSerializer::ReadVector4(*CameraJson, "Color", ColorVec, CameraNotify->VignetteSettings.Color.ToFVector4(), false))
                        {
                            CameraNotify->VignetteSettings.Color = FLinearColor(ColorVec.X, ColorVec.Y, ColorVec.Z, ColorVec.W);
                        }
                        { float TempPriority = static_cast<float>(CameraNotify->VignetteSettings.Priority); FJsonSerializer::ReadFloat(*CameraJson, "Priority", TempPriority, TempPriority, false); CameraNotify->VignetteSettings.Priority = static_cast<int32>(TempPriority); }
                        break;
                    case ECameraNotifyEffect::Gamma:
                        FJsonSerializer::ReadFloat(*CameraJson, "Gamma", CameraNotify->GammaSettings.Gamma, CameraNotify->GammaSettings.Gamma, false);
                        break;
                    case ECameraNotifyEffect::DOF:
                        FJsonSerializer::ReadFloat(*CameraJson, "FocalDistance", CameraNotify->DOFSettings.FocalDistance, CameraNotify->DOFSettings.FocalDistance, false);
                        FJsonSerializer::ReadFloat(*CameraJson, "FocalRegion", CameraNotify->DOFSettings.FocalRegion, CameraNotify->DOFSettings.FocalRegion, false);
                        FJsonSerializer::ReadFloat(*CameraJson, "NearTransition", CameraNotify->DOFSettings.NearTransition, CameraNotify->DOFSettings.NearTransition, false);
                        FJsonSerializer::ReadFloat(*CameraJson, "FarTransition", CameraNotify->DOFSettings.FarTransition, CameraNotify->DOFSettings.FarTransition, false);
                        FJsonSerializer::ReadFloat(*CameraJson, "MaxNearBlur", CameraNotify->DOFSettings.MaxNearBlur, CameraNotify->DOFSettings.MaxNearBlur, false);
                        FJsonSerializer::ReadFloat(*CameraJson, "MaxFarBlur", CameraNotify->DOFSettings.MaxFarBlur, CameraNotify->DOFSettings.MaxFarBlur, false);
                        { float TempPriority = static_cast<float>(CameraNotify->DOFSettings.Priority); FJsonSerializer::ReadFloat(*CameraJson, "Priority", TempPriority, TempPriority, false); CameraNotify->DOFSettings.Priority = static_cast<int32>(TempPriority); }
                        break;
                    default:
                        break;
                    }
                }
            }

            Evt.Notify = CameraNotify;
            Evt.NotifyState = nullptr;
        }
        else
        {
            // Fallback: generic notify
            if (!ClassStr.empty())
            {
                if (UClass* Cls = UClass::FindClass(FName(ClassStr)))
                {
                    UObject* Obj = ObjectFactory::NewObject(Cls);
                    Evt.Notify = Cast<UAnimNotify>(Obj);
                }
            }
            if (!Evt.Notify)
            {
                Evt.Notify = NewObject<UAnimNotify>();
            }
        }

        Notifies.Add(Evt);
    }

    return true;
}

FString UAnimSequenceBase::GetNotifyPath() const
{
    FString Path = GetFilePath();
    FWideString WString = UTF8ToWide(NormalizePath(Path));
    std::filesystem::path p(WString);
    FString stem = WideToUTF8(p.stem().wstring());
    return NormalizePath(GDataDir + "/" + stem + ".anim.json");
}
