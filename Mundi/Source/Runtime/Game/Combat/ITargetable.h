#pragma once

#include "Vector.h"

class AActor;

// ============================================================================
// ITargetable - Interface for actors that can be locked onto
// ============================================================================
// Usage:
//   class AEnemyBase : public ACharacter, public IDamageable, public ITargetable
//   class ABossEnemy : public AEnemyBase  (inherits ITargetable)
// ============================================================================

class ITargetable
{
public:
    virtual ~ITargetable() = default;

    // ========================================================================
    // Required Methods
    // ========================================================================

    /**
     * Returns true if this target can currently be locked onto.
     * (Check alive state, visibility, etc.)
     */
    virtual bool CanBeTargeted() const { return true; }

    /**
     * Returns the world location of the target point.
     * Usually chest/center height for humanoids.
     */
    virtual FVector GetTargetLocation() const = 0;

    // ========================================================================
    // Optional Methods (with default implementations)
    // ========================================================================

    /**
     * Returns the height offset for the lock-on point.
     * Default: 0 (uses actor location directly)
     */
    virtual float GetTargetHeight() const { return 0.0f; }

    /**
     * Returns the target priority (higher = preferred when multiple targets).
     * Default: 1.0, Bosses might use 10.0
     */
    virtual float GetTargetPriority() const { return 1.0f; }

    /**
     * Returns the maximum distance this target can be locked from.
     * Default: 2000 units
     */
    virtual float GetMaxLockOnDistance() const { return 2000.0f; }

    /**
     * Called when this target is locked onto by the player.
     */
    virtual void OnTargetLocked() {}

    /**
     * Called when this target is no longer locked.
     */
    virtual void OnTargetUnlocked() {}
};

// ============================================================================
// Utility: ITargetable casting helper
// ============================================================================

/**
 * Casts an Actor to ITargetable.
 * @param Actor The actor to cast
 * @return ITargetable pointer (nullptr if not targetable)
 */
template<typename T>
ITargetable* GetTargetable(T* Actor)
{
    return dynamic_cast<ITargetable*>(Actor);
}
