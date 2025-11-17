#include "pch.h"
#include "PlayerController.h"
#include "Pawn.h"

APlayerController::APlayerController()
{
}

APlayerController::~APlayerController()
{
}

void APlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Pawn == nullptr) return;

	// 입력 처리 (Move)
	ProcessMovementInput(DeltaSeconds);

	// 입력 처리 (Look/Turn)
	ProcessRotationInput(DeltaSeconds);
}

void APlayerController::SetupInput()
{
	// InputManager에 키 바인딩
}

void APlayerController::ProcessMovementInput(float DeltaTime)
{
	FVector InputDir = FVector::Zero();

	// InputManager 사용
	UInputManager& InputManager =  UInputManager::GetInstance();
	if (InputManager.IsKeyPressed('W'))
	{
		InputDir.X += 1.0f;
	}
	if (InputManager.IsKeyPressed('S'))
	{
		InputDir.X -= 1.0f;
	}
	if (InputManager.IsKeyPressed('D'))
	{
		InputDir.Y += 1.0f;
	}
	if (InputManager.IsKeyPressed('A'))
	{
		InputDir.Y -= 1.0f;
	}

	if (!InputDir.IsZero())
	{
		InputDir.Normalize();

		// controller의 회전을  inputDir에  적용시켜준다.
		FMatrix RotMatrix = GetControlRotation().ToMatrix(); 
		FVector WorldDir = RotMatrix.TransformVector(InputDir);
	
		Pawn->AddMovementInput(WorldDir * DeltaTime);
	}
}

void APlayerController::ProcessRotationInput(float DeltaTime)
{
	UInputManager& InputManager = UInputManager::GetInstance();
	float MouseX = InputManager.GetMousePosition().X;
	float MouseY = InputManager.GetMousePosition().Y;

	if (MouseX != 0.0f || MouseY != 0.0f)
	{
		FQuat NewRot = GetControlRotation();
		
		// Yaw, Pitch
		/*
		NewRot.Yaw += MouseX * Sensitivity;
        NewRot.Pitch += MouseY * Sensitivity;
		*/
		SetControlRotation(NewRot);
	}

}
