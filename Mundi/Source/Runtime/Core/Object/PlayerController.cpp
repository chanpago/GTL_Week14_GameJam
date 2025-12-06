#include "pch.h"
#include "PlayerController.h"
#include "Pawn.h"
#include "CameraComponent.h"
#include "SpringArmComponent.h"
#include "TargetingComponent.h"
#include <windows.h>
#include <cmath>
#include "Character.h"

APlayerController::APlayerController()
{
    // Create targeting component
    TargetingComponent = CreateDefaultSubobject<UTargetingComponent>("TargetingComponent");
}

APlayerController::~APlayerController()
{
}

void APlayerController::BeginPlay()
{
	Super::BeginPlay();

}

void APlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

    if (Pawn == nullptr) return;

	// F11을 통해서, Detail Panel 클릭이 가능해짐
    {
        UInputManager& InputManager = UInputManager::GetInstance();
        if (InputManager.IsKeyPressed(VK_F11))
        {
            bMouseLookEnabled = !bMouseLookEnabled;
            if (bMouseLookEnabled)
            {
                InputManager.SetCursorVisible(false);
                InputManager.LockCursor();
                InputManager.LockCursorToCenter();
            }
            else
            {
                InputManager.SetCursorVisible(true);
                InputManager.ReleaseCursor();
            }
        }
    }

	// 입력 처리 (Lock-on)
	ProcessLockOnInput();

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
	UInputManager& InputManager = UInputManager::GetInstance();

	FVector InputDir = FVector::Zero();

	if (InputManager.IsKeyDown('W'))
	{
		InputDir.X += 1.0f;
	}
	if (InputManager.IsKeyDown('S'))
	{
		InputDir.X -= 1.0f;
	}
	if (InputManager.IsKeyDown('D'))
	{
		InputDir.Y += 1.0f;
	}
	if (InputManager.IsKeyDown('A'))
	{
		InputDir.Y -= 1.0f;
	}

	if (!InputDir.IsZero())
	{
		InputDir.Normalize();

		// Calculate world movement direction based on camera
		FVector WorldDir;
		bool bIsLockedOn = TargetingComponent && TargetingComponent->IsLockedOn();

		if (bIsLockedOn)
		{
			// When locked on, use SpringArm rotation (actual camera facing target)
			// This makes A/D strafe relative to the lock-on camera
			if (UActorComponent* C = Pawn->GetComponent(USpringArmComponent::StaticClass()))
			{
				if (USpringArmComponent* SpringArm = Cast<USpringArmComponent>(C))
				{
					FVector SpringArmEuler = SpringArm->GetWorldRotation().ToEulerZYXDeg();
					FQuat YawOnlyRotation = FQuat::MakeFromEulerZYX(FVector(0.0f, 0.0f, SpringArmEuler.Z));
					WorldDir = YawOnlyRotation.RotateVector(InputDir);
				}
				else
				{
					WorldDir = InputDir;
				}
			}
			else
			{
				WorldDir = InputDir;
			}
		}
		else
		{
			// Normal movement: use ControlRotation
			FVector ControlEuler = GetControlRotation().ToEulerZYXDeg();
			FQuat YawOnlyRotation = FQuat::MakeFromEulerZYX(FVector(0.0f, 0.0f, ControlEuler.Z));
			WorldDir = YawOnlyRotation.RotateVector(InputDir);
		}

		WorldDir.Z = 0.0f; // 수평 이동만
		WorldDir.Normalize();

		// Lock-on 상태에 따라 회전 처리
		if (bIsLockedOn)
		{
			// Strafing: 타겟을 바라보면서 이동
			ProcessLockedMovement(DeltaTime, WorldDir);
		}
		else
		{
			// 일반 이동: 이동 방향으로 캐릭터 회전
			float TargetYaw = std::atan2(WorldDir.Y, WorldDir.X) * (180.0f / PI);
			FQuat TargetRotation = FQuat::MakeFromEulerZYX(FVector(0.0f, 0.0f, TargetYaw));

			// 부드러운 회전 (보간)
			FQuat CurrentRotation = Pawn->GetActorRotation();
			FQuat NewRotation = FQuat::Slerp(CurrentRotation, TargetRotation, FMath::Clamp(DeltaTime * 3.0f, 0.0f, 1.0f));
			Pawn->SetActorRotation(NewRotation);
		}

		// 이동 적용
		Pawn->AddMovementInput(WorldDir * (Pawn->GetVelocity() * DeltaTime));
	}

    // 점프 처리
    if (InputManager.IsKeyPressed(VK_SPACE)) {
        if (auto* Character = Cast<ACharacter>(Pawn)) {
            Character->Jump();
        }
    }
    if (InputManager.IsKeyReleased(VK_SPACE)) {
        if (auto* Character = Cast<ACharacter>(Pawn)) {
            Character->StopJumping();
        }
    }
}

void APlayerController::ProcessRotationInput(float DeltaTime)
{
    UInputManager& InputManager = UInputManager::GetInstance();
    if (!bMouseLookEnabled)
        return;

    bool bIsLockedOn = TargetingComponent && TargetingComponent->IsLockedOn();

    // Skip mouse input when locked on (Dark Souls style - camera auto-tracks target)
    if (!bIsLockedOn)
    {
        FVector2D MouseDelta = InputManager.GetMouseDelta();

        // 마우스 입력이 있을 때만 ControlRotation 업데이트
        if (MouseDelta.X != 0.0f || MouseDelta.Y != 0.0f)
        {
            const float Sensitivity = 0.1f;

            FVector Euler = GetControlRotation().ToEulerZYXDeg();
            // Yaw (좌우 회전)
            Euler.Z += MouseDelta.X * Sensitivity;

            // Pitch (상하 회전)
            Euler.Y += MouseDelta.Y * Sensitivity;
            Euler.Y = FMath::Clamp(Euler.Y, -89.0f, 89.0f);

            // Roll 방지
            Euler.X = 0.0f;

            FQuat NewControlRotation = FQuat::MakeFromEulerZYX(Euler);
            SetControlRotation(NewControlRotation);
        }
    }

    // SpringArm 처리
    if (UActorComponent* C = Pawn->GetComponent(USpringArmComponent::StaticClass()))
    {
        if (USpringArmComponent* SpringArm = Cast<USpringArmComponent>(C))
        {
            if (bIsLockedOn)
            {
                // Lock-on 상태: SpringArm에 타겟 설정 (자동 추적)
                SpringArm->SetLockOnTarget(TargetingComponent->GetLockedTarget());
            }
            else
            {
                // 일반 상태: 수동으로 SpringArm 회전 설정
                SpringArm->ClearLockOnTarget();
                FVector Euler = GetControlRotation().ToEulerZYXDeg();
                FQuat SpringArmRot = FQuat::MakeFromEulerZYX(FVector(0.0f, Euler.Y, Euler.Z));
                SpringArm->SetWorldRotation(SpringArmRot);
            }
        }
    }
}

void APlayerController::ProcessLockOnInput()
{
    if (!TargetingComponent) return;

    UInputManager& InputManager = UInputManager::GetInstance();

    // Ctrl: Lock-on 토글
    if (InputManager.IsKeyPressed(VK_CONTROL))
    {
        bool bWasLockedOn = TargetingComponent->IsLockedOn();

        TargetingComponent->ToggleLockOn();

        bool bIsNowLockedOn = TargetingComponent->IsLockedOn();

        // When unlocking, sync ControlRotation to current camera rotation (prevents snap)
        if (bWasLockedOn && !bIsNowLockedOn && Pawn)
        {
            if (UActorComponent* C = Pawn->GetComponent(USpringArmComponent::StaticClass()))
            {
                if (USpringArmComponent* SpringArm = Cast<USpringArmComponent>(C))
                {
                    // Get current SpringArm rotation and set it as ControlRotation
                    FQuat SpringArmRot = SpringArm->GetWorldRotation();
                    SetControlRotation(SpringArmRot);
                }
            }
        }
    }

    // Q/E: 타겟 전환 (여러 타겟이 있을 때)
    if (TargetingComponent->IsLockedOn())
    {
        if (InputManager.IsKeyPressed('Q'))
        {
            TargetingComponent->SwitchTarget(ETargetSwitchDirection::Left);
        }
        if (InputManager.IsKeyPressed('E'))
        {
            TargetingComponent->SwitchTarget(ETargetSwitchDirection::Right);
        }
    }

    // 애니메이션 그래프용 bIsLockOn 동기화
    bIsLockOn = TargetingComponent->IsLockedOn();
}

void APlayerController::ProcessLockedMovement(float DeltaTime, const FVector& WorldMoveDir)
{
    if (!Pawn || !TargetingComponent || !TargetingComponent->IsLockedOn()) return;

    // 타겟 방향으로 캐릭터 회전
    float TargetYaw = TargetingComponent->GetYawToTarget();
    FQuat TargetRotation = FQuat::MakeFromEulerZYX(FVector(0.0f, 0.0f, TargetYaw));

    // 부드러운 회전 (보간)
    FQuat CurrentRotation = Pawn->GetActorRotation();
    FQuat NewRotation = FQuat::Slerp(CurrentRotation, TargetRotation,
                                     FMath::Clamp(DeltaTime * CharacterRotationSpeed, 0.0f, 1.0f));
    Pawn->SetActorRotation(NewRotation);
}
