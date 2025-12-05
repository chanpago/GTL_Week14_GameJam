#pragma once

#include "Actor.h"
#include "AGameStateBase.generated.h"

class UWorld;
class AGameModeBase;
class APlayerController;
class APawn;

// Engine-agnostic base game state. Holds links and generic lifecycle.
class AGameStateBase : public AActor
{
public:
    GENERATED_REFLECTION_BODY()

    AGameStateBase() = default;
    ~AGameStateBase() override = default;

    // Lifecycle
    void BeginPlay() override;
    void EndPlay() override;
    void Tick(float DeltaTime) override;

    // Initialization and links
    void Initialize(UWorld* InWorld, AGameModeBase* InGameMode);
    UWorld* GetWorldUnsafe() const { return World; }
    AGameModeBase* GetGameMode() const { return GameMode; }
    APlayerController* GetMainController() const { return MainController; }
    APawn* GetMainPawn() const { return MainPawn; }

    // Pause interface
    void SetPaused(bool bInPaused) { bPaused = bInPaused; }
    bool IsPaused() const { return bPaused; }

    // Notifications (override in derived game-specific state)
    virtual void OnPlayerLogin(APlayerController* InController);
    virtual void OnPawnPossessed(APawn* InPawn);
    virtual void OnPlayerDied();

protected:
    // Per-frame hook for derived classes to implement logic
    virtual void HandleStateTick(float DeltaTime) {}

protected:
    // References (not owned)
    UWorld* World = nullptr;
    AGameModeBase* GameMode = nullptr;
    APlayerController* MainController = nullptr;
    APawn* MainPawn = nullptr;

    // Generic state
    bool bPaused = false;
    float StateTimeSeconds = 0.0f; // increments in Tick
};

