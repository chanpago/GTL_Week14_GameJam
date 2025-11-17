#include "pch.h"
#include "Pawn.h"

APawn::APawn()
{
}

void APawn::Tick(float DeltaTime)
{
}

void APawn::BeginPlay()
{
	Super::BeginPlay();
}

void APawn::PossessedBy(AController* NewController)
{
	Controller = NewController;
}

void APawn::UnPossessed()
{
	Controller = nullptr;
}

void APawn::AddMovementInput(FVector Direction, float Scale)
{
	InternalMovementInputVector += Direction * Scale;
}

APawn::~APawn()
{
}
