#pragma once

#include "ActorComponent.h"
#include "UTargetingComponent.generated.h"

class AActor;
class ITargetable;
class APawn;

// ============================================================================
// Target Switch Direction
// ============================================================================
enum class ETargetSwitchDirection
{
    Left,
    Right,
    Nearest
};

// ============================================================================
// UTargetingComponent - Souls-like lock-on targeting system
// ============================================================================
UCLASS(DisplayName = "Targeting Component", Description = "Souls-like lock-on targeting system")
class UTargetingComponent : public UActorComponent
{
public:
    GENERATED_REFLECTION_BODY()

    UTargetingComponent();
    virtual ~UTargetingComponent() = default;

    // ========================================================================
    // Lifecycle
    // ========================================================================
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime) override;

    // ========================================================================
    // Lock-On Control
    // ========================================================================

    /** Toggle lock-on (finds nearest target or unlocks current) */
    void ToggleLockOn();

    /** Lock onto a specific target */
    bool LockOnToTarget(AActor* Target);

    /** Unlock current target */
    void UnlockTarget();

    /** Switch target in the specified direction (screen-space) */
    void SwitchTarget(ETargetSwitchDirection Direction);

    // ========================================================================
    // State Queries
    // ========================================================================

    bool IsLockedOn() const { return LockedTarget != nullptr; }
    AActor* GetLockedTarget() const { return LockedTarget; }

    /** Get target location (from ITargetable or actor location) */
    FVector GetTargetLocation() const;

    /** Get direction from owner to target (normalized, horizontal) */
    FVector GetDirectionToTarget() const;

    /** Get yaw angle (degrees) to face the target */
    float GetYawToTarget() const;

    // ========================================================================
    // Target Discovery
    // ========================================================================

    /** Find all valid targets in range */
    TArray<AActor*> FindPotentialTargets() const;

    /** Find the best target to lock onto (nearest in view) */
    AActor* FindBestTarget() const;

    /** Check if a specific actor is a valid target */
    bool IsValidTarget(AActor* Target) const;

    // ========================================================================
    // Configuration
    // ========================================================================

    UPROPERTY(EditAnywhere, Category = "Targeting")
    float MaxLockOnRange = 1500.0f;

    UPROPERTY(EditAnywhere, Category = "Targeting")
    float MaxLockOnAngle = 60.0f;  // Degrees from forward direction

    UPROPERTY(EditAnywhere, Category = "Targeting")
    float TargetLostRange = 2000.0f;  // Auto-unlock if target goes beyond this

    UPROPERTY(EditAnywhere, Category = "Targeting")
    float TargetValidationInterval = 0.1f;  // How often to check if target is still valid

protected:
    // ========================================================================
    // Internal Methods
    // ========================================================================

    /** Validate current target is still lockable */
    void ValidateCurrentTarget();

    /** Score a potential target (higher = better) */
    float ScoreTarget(AActor* Target) const;

    /** Check line of sight to target (optional, can be disabled) */
    bool HasLineOfSight(AActor* Target) const;

private:
    AActor* LockedTarget = nullptr;
    float ValidationTimer = 0.0f;

    // Cached reference to owner pawn
    mutable APawn* OwnerPawn = nullptr;

    /** Ensure OwnerPawn is valid (lazy initialization) */
    void EnsureOwnerPawn() const;
};
