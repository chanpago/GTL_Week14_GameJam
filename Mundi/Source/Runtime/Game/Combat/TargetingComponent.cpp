#include "pch.h"
#include "TargetingComponent.h"
#include "ITargetable.h"
#include "Pawn.h"
#include "Actor.h"
#include "World.h"
#include "EnemyBase.h"
#include "PlayerController.h"
#include <cmath>

UTargetingComponent::UTargetingComponent()
{
    bCanEverTick = true;
}

void UTargetingComponent::BeginPlay()
{
    Super::BeginPlay();

    // Cache owner pawn reference
    // The component is owned by PlayerController, so get Pawn from it
    if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
    {
        OwnerPawn = PC->GetPawn();
        if (OwnerPawn)
        {
            UE_LOG("[TargetingComponent] BeginPlay: OwnerPawn set to %s (via PlayerController)", OwnerPawn->GetName().c_str());
        }
        else
        {
            UE_LOG("[TargetingComponent] BeginPlay: WARNING - PlayerController's Pawn is null!");
        }
    }
    else
    {
        // Fallback: try direct cast in case component is attached to Pawn
        OwnerPawn = Cast<APawn>(GetOwner());
        if (OwnerPawn)
        {
            UE_LOG("[TargetingComponent] BeginPlay: OwnerPawn set to %s (direct)", OwnerPawn->GetName().c_str());
        }
        else
        {
            UE_LOG("[TargetingComponent] BeginPlay: WARNING - OwnerPawn is null! Owner is %s",
                   GetOwner() ? GetOwner()->GetName().c_str() : "null");
        }
    }
}

void UTargetingComponent::TickComponent(float DeltaTime)
{
    Super::TickComponent(DeltaTime);

    // Periodically validate current target
    if (LockedTarget)
    {
        ValidationTimer += DeltaTime;
        if (ValidationTimer >= TargetValidationInterval)
        {
            ValidateCurrentTarget();
            ValidationTimer = 0.0f;
        }
    }
}

// ============================================================================
// Lock-On Control
// ============================================================================

void UTargetingComponent::ToggleLockOn()
{
    UE_LOG("[TargetingComponent] ToggleLockOn called, currently locked: %d", IsLockedOn());

    if (IsLockedOn())
    {
        UnlockTarget();
        UE_LOG("[TargetingComponent] Unlocked target");
    }
    else
    {
        TArray<AActor*> PotentialTargets = FindPotentialTargets();
        UE_LOG("[TargetingComponent] Found %d potential targets", PotentialTargets.Num());

        AActor* BestTarget = FindBestTarget();
        if (BestTarget)
        {
            LockOnToTarget(BestTarget);
            UE_LOG("[TargetingComponent] Locked onto target: %s", BestTarget->GetName().c_str());
        }
        else
        {
            UE_LOG("[TargetingComponent] No valid target found");
        }
    }
}

bool UTargetingComponent::LockOnToTarget(AActor* Target)
{
    if (!Target || !IsValidTarget(Target))
        return false;

    AActor* OldTarget = LockedTarget;

    // Notify old target it's being unlocked
    if (OldTarget)
    {
        if (ITargetable* Targetable = GetTargetable(OldTarget))
        {
            Targetable->OnTargetUnlocked();
        }
    }

    LockedTarget = Target;

    // Notify new target it's being locked
    if (ITargetable* Targetable = GetTargetable(Target))
    {
        Targetable->OnTargetLocked();
    }

    return true;
}

void UTargetingComponent::UnlockTarget()
{
    if (LockedTarget)
    {
        if (ITargetable* Targetable = GetTargetable(LockedTarget))
        {
            Targetable->OnTargetUnlocked();
        }
        LockedTarget = nullptr;
    }
}

void UTargetingComponent::SwitchTarget(ETargetSwitchDirection Direction)
{
    if (!IsLockedOn()) return;

    TArray<AActor*> Targets = FindPotentialTargets();
    if (Targets.Num() <= 1) return;

    // For now, simple index-based switching
    // TODO: Implement screen-space sorting for proper left/right switching
    int32 CurrentIndex = Targets.Find(LockedTarget);
    if (CurrentIndex == INDEX_NONE) return;

    int32 NewIndex = CurrentIndex;
    if (Direction == ETargetSwitchDirection::Left)
    {
        NewIndex = (CurrentIndex - 1 + Targets.Num()) % Targets.Num();
    }
    else if (Direction == ETargetSwitchDirection::Right)
    {
        NewIndex = (CurrentIndex + 1) % Targets.Num();
    }

    if (NewIndex != CurrentIndex)
    {
        LockOnToTarget(Targets[NewIndex]);
    }
}

// ============================================================================
// State Queries
// ============================================================================

FVector UTargetingComponent::GetTargetLocation() const
{
    if (!LockedTarget) return FVector::Zero();

    // Try to get location from ITargetable interface
    if (ITargetable* Targetable = GetTargetable(LockedTarget))
    {
        return Targetable->GetTargetLocation();
    }

    // Fallback to actor location
    return LockedTarget->GetActorLocation();
}

FVector UTargetingComponent::GetDirectionToTarget() const
{
    // X-forward in Z-up left-handed coordinate system
    EnsureOwnerPawn();
    if (!LockedTarget || !OwnerPawn) return FVector(1, 0, 0);

    FVector ToTarget = GetTargetLocation() - OwnerPawn->GetActorLocation();
    ToTarget.Z = 0;  // Horizontal only

    if (ToTarget.IsZero()) return FVector(1, 0, 0);

    ToTarget.Normalize();
    return ToTarget;
}

float UTargetingComponent::GetYawToTarget() const
{
    FVector Dir = GetDirectionToTarget();
    return std::atan2(Dir.Y, Dir.X) * (180.0f / PI);
}

// ============================================================================
// Target Discovery
// ============================================================================

TArray<AActor*> UTargetingComponent::FindPotentialTargets() const
{
    TArray<AActor*> ValidTargets;
    UWorld* World = GetWorld();
    EnsureOwnerPawn();
    if (!World || !OwnerPawn) return ValidTargets;

    // Find all enemies in the world
    TArray<AEnemyBase*> Enemies = World->FindActors<AEnemyBase>();

    for (AEnemyBase* Enemy : Enemies)
    {
        if (IsValidTarget(Enemy))
        {
            ValidTargets.Add(Enemy);
        }
    }

    return ValidTargets;
}

AActor* UTargetingComponent::FindBestTarget() const
{
    TArray<AActor*> Targets = FindPotentialTargets();
    if (Targets.Num() == 0) return nullptr;

    AActor* BestTarget = nullptr;
    float BestScore = -1.0f;

    for (AActor* Target : Targets)
    {
        float Score = ScoreTarget(Target);
        if (Score > BestScore)
        {
            BestScore = Score;
            BestTarget = Target;
        }
    }

    return BestTarget;
}

bool UTargetingComponent::IsValidTarget(AActor* Target) const
{
    EnsureOwnerPawn();
    if (!Target || !OwnerPawn)
    {
        UE_LOG("[TargetingComponent] IsValidTarget: Target or OwnerPawn is null");
        return false;
    }

    // Must implement ITargetable
    ITargetable* Targetable = GetTargetable(Target);
    if (!Targetable)
    {
        UE_LOG("[TargetingComponent] IsValidTarget: %s does not implement ITargetable", Target->GetName().c_str());
        return false;
    }

    // Must be targetable (alive, etc.)
    if (!Targetable->CanBeTargeted())
    {
        UE_LOG("[TargetingComponent] IsValidTarget: %s CanBeTargeted() returned false", Target->GetName().c_str());
        return false;
    }

    // Check distance
    FVector ToTarget = Target->GetActorLocation() - OwnerPawn->GetActorLocation();
    float Distance = ToTarget.Size();
    if (Distance > MaxLockOnRange)
    {
        UE_LOG("[TargetingComponent] IsValidTarget: %s too far (%.1f > %.1f)", Target->GetName().c_str(), Distance, MaxLockOnRange);
        return false;
    }

    // Check angle from camera direction (ControlRotation), not player facing
    ToTarget.Z = 0;
    if (!ToTarget.IsZero())
    {
        ToTarget.Normalize();

        // Get camera forward from PlayerController's ControlRotation
        FVector CameraForward = FVector(1, 0, 0);  // Default forward
        if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
        {
            FQuat ControlRot = PC->GetControlRotation();
            CameraForward = ControlRot.RotateVector(FVector(1, 0, 0));
        }
        CameraForward.Z = 0;
        CameraForward.Normalize();

        float Dot = FVector::Dot(CameraForward, ToTarget);
        float Angle = std::acos(FMath::Clamp(Dot, -1.0f, 1.0f)) * (180.0f / PI);

        if (Angle > MaxLockOnAngle)
        {
            UE_LOG("[TargetingComponent] IsValidTarget: %s angle too wide (%.1f > %.1f)", Target->GetName().c_str(), Angle, MaxLockOnAngle);
            return false;
        }
    }

    return true;
}

// ============================================================================
// Internal Methods
// ============================================================================

void UTargetingComponent::ValidateCurrentTarget()
{
    if (!LockedTarget) return;

    // Check if target is still valid via interface
    ITargetable* Targetable = GetTargetable(LockedTarget);
    if (!Targetable || !Targetable->CanBeTargeted())
    {
        UnlockTarget();
        return;
    }

    // Check if target is too far away
    EnsureOwnerPawn();
    if (OwnerPawn)
    {
        float Distance = (LockedTarget->GetActorLocation() - OwnerPawn->GetActorLocation()).Size();
        if (Distance > TargetLostRange)
        {
            UnlockTarget();
        }
    }
}

float UTargetingComponent::ScoreTarget(AActor* Target) const
{
    EnsureOwnerPawn();
    if (!Target || !OwnerPawn) return -1.0f;

    FVector ToTarget = Target->GetActorLocation() - OwnerPawn->GetActorLocation();
    float Distance = ToTarget.Size();

    if (Distance > MaxLockOnRange) return -1.0f;

    // Angle check using camera direction
    ToTarget.Z = 0;
    if (ToTarget.IsZero()) return -1.0f;

    ToTarget.Normalize();

    // Get camera forward from PlayerController's ControlRotation
    FVector CameraForward = FVector(1, 0, 0);  // Default forward
    if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
    {
        FQuat ControlRot = PC->GetControlRotation();
        CameraForward = ControlRot.RotateVector(FVector(1, 0, 0));
    }
    CameraForward.Z = 0;
    CameraForward.Normalize();

    float Dot = FVector::Dot(CameraForward, ToTarget);
    float Angle = std::acos(FMath::Clamp(Dot, -1.0f, 1.0f)) * (180.0f / PI);

    if (Angle > MaxLockOnAngle) return -1.0f;

    // Score: prefer closer and more centered targets
    float DistanceScore = 1.0f - (Distance / MaxLockOnRange);
    float AngleScore = 1.0f - (Angle / MaxLockOnAngle);

    // Get priority from ITargetable (bosses have higher priority)
    float Priority = 1.0f;
    if (ITargetable* Targetable = GetTargetable(Target))
    {
        Priority = Targetable->GetTargetPriority();
    }

    return (DistanceScore * 0.4f + AngleScore * 0.4f) * Priority;
}

bool UTargetingComponent::HasLineOfSight(AActor* Target) const
{
    // TODO: Implement raycast check for line of sight
    // For now, always return true
    return true;
}

void UTargetingComponent::EnsureOwnerPawn() const
{
    if (OwnerPawn) return;

    // Try to get Pawn from PlayerController owner
    if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
    {
        OwnerPawn = PC->GetPawn();
        if (OwnerPawn)
        {
            UE_LOG("[TargetingComponent] EnsureOwnerPawn: Got Pawn %s from PlayerController", OwnerPawn->GetName().c_str());
        }
    }
}
