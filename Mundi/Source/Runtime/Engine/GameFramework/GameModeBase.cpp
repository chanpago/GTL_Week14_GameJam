#include "pch.h"
#include "GameModeBase.h"
#include "PlayerController.h"
#include "Pawn.h"
#include "World.h"
#include "CameraComponent.h"
#include "SpringArmComponent.h"
#include "PlayerCameraManager.h"
#include "Character.h"
#include "Source/Runtime/Core/Misc/PathUtils.h"
#include "GameState.h"

AGameModeBase::AGameModeBase()
{
	//DefaultPawnClass = APawn::StaticClass();
	DefaultPawnClass = ACharacter::StaticClass();
	PlayerControllerClass = APlayerController::StaticClass();
    GameStateClass = AGameState::StaticClass();
}

AGameModeBase::~AGameModeBase()
{
}

void AGameModeBase::StartPlay()
{
	// Ensure GameState exists (spawn using configured class)
	if (!GameStateClass)
	{
		GameStateClass = AGameState::StaticClass();
	}
	if (!GameState)
	{
		if (AActor* GSActor = GetWorld()->SpawnActor(GameStateClass, FTransform()))
		{
			GameState = Cast<AGameStateBase>(GSActor);
			if (GameState)
			{
				GameState->Initialize(GetWorld(), this);
			}
		}
	}

	// Player login + spawn/pawn possession
	Login();
	PostLogin(PlayerController);

	// Notify GameState of initial controller/pawn
	if (GameState && PlayerController)
	{
		GameState->OnPlayerLogin(PlayerController);
		if (APawn* P = PlayerController->GetPawn())
		{
			GameState->OnPawnPossessed(P);
		}
	}

	// Initialize flow: PIE shows StartMenu; non-PIE goes straight to intro/fight
	if (GameState)
	{
		if (GWorld && GWorld->bPie)
		{
			if (AGameState* GS = Cast<AGameState>(GameState))
			{
				GS->EnterStartMenu();
			}
		}
		else
		{
			if (AGameState* GS = Cast<AGameState>(GameState))
			{
				GS->EnterBossIntro();
			}
		}
	}
}

APlayerController* AGameModeBase::Login()
{
	if (PlayerControllerClass)
	{
		PlayerController = Cast<APlayerController>(GWorld->SpawnActor(PlayerControllerClass, FTransform())); 
	}
	else
	{
		PlayerController = GWorld->SpawnActor<APlayerController>(FTransform());
	}

	return PlayerController;
}

void AGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	// 스폰 위치 찾기
	AActor* StartSpot = FindPlayerStart(NewPlayer);
	APawn* NewPawn = NewPlayer->GetPawn();

	// Pawn이 없으면 생성
    if (!NewPawn && NewPlayer)
    {
        // Try spawning a default prefab as the player's pawn first
        {
            FWideString PrefabPath = UTF8ToWide(GDataDir) + L"/Prefabs/Shinobi.prefab";
            if (AActor* PrefabActor = GWorld->SpawnPrefabActor(PrefabPath))
            {
                if (APawn* PrefabPawn = Cast<APawn>(PrefabActor))
                {
                    NewPawn = PrefabPawn;
                }
            }
        }

        // Fallback to default pawn class if prefab did not yield a pawn
        if (!NewPawn)
        {
            NewPawn = SpawnDefaultPawnFor(NewPlayer, StartSpot);
        }

        if (NewPawn)
        {
            NewPlayer->Possess(NewPawn);
        }

    }

	// SpringArm + Camera 구조로 부착
	if (NewPawn && !NewPawn->GetComponent(USpringArmComponent::StaticClass()))
	{
		// 1. SpringArm 생성 및 RootComponent에 부착
		USpringArmComponent* SpringArm = nullptr;
		if (UActorComponent* SpringArmComp = NewPawn->AddNewComponent(USpringArmComponent::StaticClass(), NewPawn->GetRootComponent()))
		{
			SpringArm = Cast<USpringArmComponent>(SpringArmComp);
			SpringArm->SetRelativeLocation(FVector(0, 0, 0.8f));  // 캐릭터 머리 위쪽
			SpringArm->SetTargetArmLength(3.0f);                  // 카메라 거리 (스프링 암에서 카메라 컴포넌트까지의)
			SpringArm->SetDoCollisionTest(true);                  // 충돌 체크 활성화
			SpringArm->SetUsePawnControlRotation(true);           // 컨트롤러 회전 사용
		}

		// 2. Camera를 SpringArm에 부착
		if (SpringArm)
		{
			if (UActorComponent* CameraComp = NewPawn->AddNewComponent(UCameraComponent::StaticClass(), SpringArm))
			{
				auto* Camera = Cast<UCameraComponent>(CameraComp);
				// 카메라는 SpringArm 끝에 위치 (SpringArm이 위치 계산)
				Camera->SetRelativeLocation(FVector(0, 0, 0));
				Camera->SetRelativeRotationEuler(FVector(0, 0, 0));
			}
		}
	}

	if (auto* PCM = GWorld->GetPlayerCameraManager())
	{
		if (auto* Camera = Cast<UCameraComponent>(NewPawn->GetComponent(UCameraComponent::StaticClass())))
		{
			PCM->SetViewCamera(Camera);
		}
	}

	// Possess를 수행 
	if (NewPlayer)
	{
		NewPlayer->Possess(NewPlayer->GetPawn());
	}

	// Inform GameState of the possessed pawn
	if (GameState && NewPlayer)
	{
		if (APawn* P = NewPlayer->GetPawn())
		{
			GameState->OnPawnPossessed(P);
		}
	}
}

APawn* AGameModeBase::SpawnDefaultPawnFor(AController* NewPlayer, AActor* StartSpot)
{
	// 위치 결정
	FVector SpawnLoc = FVector::Zero();
	FQuat SpawnRot = FQuat::Identity();

	if (StartSpot)
	{
		SpawnLoc = StartSpot->GetActorLocation();
		SpawnRot = StartSpot->GetActorRotation();
	}
	 
	APawn* ResultPawn = nullptr;
	if (DefaultPawnClass)
	{
		ResultPawn = Cast<APawn>(GetWorld()->SpawnActor(DefaultPawnClass, FTransform(SpawnLoc, SpawnRot, FVector(1, 1, 1))));
	}

	return ResultPawn;
}

AActor* AGameModeBase::FindPlayerStart(AController* Player)
{
	// TODO: PlayerStart Actor를 찾아서 반환하도록 구현 필요
	return nullptr;
}
  
