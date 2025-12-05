#include "pch.h"
#include "GameStateBase.h"
#include "World.h"
#include "GameModeBase.h"
#include "PlayerController.h"
#include "Pawn.h"

void AGameStateBase::Initialize(UWorld* InWorld, AGameModeBase* InGameMode)
{
    World = InWorld;
    GameMode = InGameMode;
}

void AGameStateBase::BeginPlay()
{
    Super::BeginPlay();
    StateTimeSeconds = 0.0f;
}

void AGameStateBase::EndPlay()
{
    Super::EndPlay();
}

void AGameStateBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bPaused)
    {
        StateTimeSeconds += DeltaTime;
    }

    HandleStateTick(DeltaTime);
}

void AGameStateBase::OnPlayerLogin(APlayerController* InController)
{
    MainController = InController;
}

void AGameStateBase::OnPawnPossessed(APawn* InPawn)
{
    MainPawn = InPawn;
}

void AGameStateBase::OnPlayerDied()
{
    // Base implementation: no-op. Override in derived game states.
}

