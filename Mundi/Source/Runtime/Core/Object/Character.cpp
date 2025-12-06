#include "pch.h"
#include "Character.h"
#include "CapsuleComponent.h"
#include "SkeletalMeshComponent.h"
#include "CharacterMovementComponent.h"
#include "StaticMeshComponent.h"
#include "ObjectMacros.h"
#include "StaticMeshActor.h"
#include "World.h" 
ACharacter::ACharacter()
{
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>("CapsuleComponent");
	//CapsuleComponent->SetSize();
    WeaponMeshComp = CreateDefaultSubobject<UStaticMeshComponent>("StaticMeshComponent");
    WeaponCollider = CreateDefaultSubobject<UCapsuleComponent>("WeaponCollider");
	SetRootComponent(CapsuleComponent);

	SubWeaponMeshComp = CreateDefaultSubobject<UStaticMeshComponent>("SubWeaponMeshComponent");

	if (SkeletalMeshComp)
	{
		SkeletalMeshComp->SetupAttachment(CapsuleComponent);

		//SkeletalMeshComp->SetRelativeLocation(FVector());
		//SkeletalMeshComp->SetRelativeScale(FVector());
	}
    if (WeaponMeshComp)
    {
        WeaponMeshComp->SetupAttachment(SkeletalMeshComp);
    }
    if (WeaponCollider)
    {
        WeaponCollider->SetupAttachment(WeaponMeshComp);
        WeaponCollider->CapsuleRadius = 0.1f;      // 반지름 10cm
        WeaponCollider->CapsuleHalfHeight = 0.5f;  // 반높이 50cm
        WeaponCollider->SetGenerateOverlapEvents(false);  // 기본 비활성화
    }
	 

	if (SubWeaponMeshComp)
	{
		SubWeaponMeshComp->SetupAttachment(SkeletalMeshComp);
	}
	 
	CharacterMovement = CreateDefaultSubobject<UCharacterMovementComponent>("CharacterMovement");
	if (CharacterMovement)
	{
		CharacterMovement->SetUpdatedComponent(CapsuleComponent);
	} 
}

ACharacter::~ACharacter()
{

}

void ACharacter::Tick(float DeltaSecond)
{
	Super::Tick(DeltaSecond);
    if (InitFrameCounter < 3) {
        InitFrameCounter++;
        return;
    }
	// 무기 트랜스폼 업데이트 (본 따라가기)
	UpdateWeaponTransform();
}

void ACharacter::BeginPlay()
{
    Super::BeginPlay();
}

void ACharacter::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        // Rebind important component pointers after load (prefab/scene)
        CapsuleComponent = nullptr;
        CharacterMovement = nullptr;
        WeaponMeshComp = nullptr;
        WeaponCollider = nullptr;
        SubWeaponMeshComp = nullptr;
        SkeletalMeshComp = nullptr;

        for (UActorComponent* Comp : GetOwnedComponents())
        {
            if (auto* Cap = Cast<UCapsuleComponent>(Comp))
            {
                // WeaponCollider와 CapsuleComponent 구분 (이름으로)
                if (Cap->GetName().find("WeaponCollider") != FString::npos)
                {
                    WeaponCollider = Cap;
                }
                else
                {
                    CapsuleComponent = Cap;
                }
            }
            else if (auto* Move = Cast<UCharacterMovementComponent>(Comp))
            {
                CharacterMovement = Move;
            }
            else if (auto* Skel = Cast<USkeletalMeshComponent>(Comp))
            {
                SkeletalMeshComp = Skel;
            }
            else if (auto* StaticMesh = Cast<UStaticMeshComponent>(Comp))
            {
                // 이름으로 구분
                FString CompName = StaticMesh->GetName();
                if (CompName.find("SubWeapon") != FString::npos)
                {
                    SubWeaponMeshComp = StaticMesh;
                }
                else if (CompName.find("Weapon") != FString::npos)
                {
                    WeaponMeshComp = StaticMesh;
                }
            }
        }

        if (CharacterMovement)
        {
            USceneComponent* Updated = CapsuleComponent ? reinterpret_cast<USceneComponent*>(CapsuleComponent)
                                                        : GetRootComponent();
            CharacterMovement->SetUpdatedComponent(Updated);
        }
    }
}

void ACharacter::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();

    CapsuleComponent = nullptr;
    CharacterMovement = nullptr;
    WeaponMeshComp = nullptr;
    WeaponCollider = nullptr;
    SubWeaponMeshComp = nullptr;
    SkeletalMeshComp = nullptr;

    for (UActorComponent* Comp : GetOwnedComponents())
    {
        if (auto* Cap = Cast<UCapsuleComponent>(Comp))
        {
            // WeaponCollider와 CapsuleComponent 구분 (이름으로)
            if (Cap->GetName().find("WeaponCollider") != FString::npos)
            {
                WeaponCollider = Cap;
            }
            else
            {
                CapsuleComponent = Cap;
            }
        }
        else if (auto* Move = Cast<UCharacterMovementComponent>(Comp))
        {
            CharacterMovement = Move;
        }
        else if (auto* Skel = Cast<USkeletalMeshComponent>(Comp))
        {
            SkeletalMeshComp = Skel;
        }
        else if (auto* StaticMesh = Cast<UStaticMeshComponent>(Comp))
        {
            // 이름으로 구분
            FString CompName = StaticMesh->GetName();
            if (CompName.find("SubWeapon") != FString::npos)
            {
                SubWeaponMeshComp = StaticMesh;
            }
            else if (CompName.find("Weapon") != FString::npos)
            {
                WeaponMeshComp = StaticMesh;
            }
        }
    }

    // Ensure movement component tracks the correct updated component
    if (CharacterMovement)
    {
        USceneComponent* Updated = CapsuleComponent ? reinterpret_cast<USceneComponent*>(CapsuleComponent)
                                                    : GetRootComponent();
        CharacterMovement->SetUpdatedComponent(Updated);
    }
}

void ACharacter::Jump()
{
	if (CharacterMovement)
	{
		CharacterMovement->DoJump();
	}
}

void ACharacter::StopJumping()
{
	if (CharacterMovement)
	{
		// 점프 scale을 조절할 때 사용,
		// 지금은 비어있음
		CharacterMovement->StopJump();
	}
}

// ============================================================================
// 무기 어태치 시스템
// ============================================================================

void ACharacter::UpdateWeaponTransform()
{
	if (!WeaponMeshComp || !SkeletalMeshComp)
	{
		return;
	}

	// 스켈레탈 메시가 로드되었는지 확인
	if (!SkeletalMeshComp->GetSkeletalMesh())
	{
		return;
	}

	// 본 인덱스 찾기
	int32 BoneIndex = SkeletalMeshComp->GetBoneIndexByName(FName(WeaponBoneName));
	if (BoneIndex < 0)
	{
		return;
	}

	// 본의 월드 트랜스폼 가져오기
	FTransform BoneTransform = SkeletalMeshComp->GetBoneWorldTransform(BoneIndex);

	// 오프셋 적용
	FVector FinalLocation = BoneTransform.Translation +
		BoneTransform.Rotation.RotateVector(WeaponOffset);

	// 회전 오프셋 적용 (오일러 → 쿼터니언)
	FQuat RotOffset = FQuat::MakeFromEulerZYX(WeaponRotationOffset);
	FQuat FinalRotation = BoneTransform.Rotation * RotOffset;

	// 무기 트랜스폼 설정
	WeaponMeshComp->SetWorldLocation(FinalLocation);
	WeaponMeshComp->SetWorldRotation(FinalRotation);
}

// ============================================================================
// 무기 충돌 시스템
// ============================================================================

void ACharacter::EnableWeaponCollision(bool bEnable)
{
	if (!WeaponCollider)
	{
		return;
	}

	// CapsuleComponent의 PhysX 기반 트리거 충돌 사용
	WeaponCollider->EnableTriggerCollision(bEnable);

	if (bEnable)
	{
		// 델리게이트 바인딩
		WeaponCollider->OnTriggerHit.Add(
			[this](AActor* OtherActor, const FVector& HitLocation)
			{
				if (OtherActor && OtherActor != this)
				{
					OnWeaponOverlap(OtherActor, HitLocation);
				}
			});
	}
	else
	{
		// 델리게이트 해제
		WeaponCollider->OnTriggerHit.Clear();
	}
}

void ACharacter::OnWeaponOverlap(AActor* OtherActor, const FVector& HitLocation)
{
	// 충돌 위치에 StaticMeshActor 스폰
	UWorld* World = GetWorld();
	if (World)
	{
		FTransform SpawnTransform;
		SpawnTransform.Translation = HitLocation;
		AStaticMeshActor* HitMarker = World->SpawnActor<AStaticMeshActor>(SpawnTransform);
		if (HitMarker)
		{
			HitMarker->GetStaticMeshComponent()->SetStaticMesh(GDataDir + "/Model/Sphere8.obj");
			HitMarker->SetActorScale(FVector(0.1f, 0.1f, 0.1f));
		}
	}

	// 델리게이트 브로드캐스트
	OnWeaponHit.Broadcast(OtherActor, HitLocation);
}

void ACharacter::UpdateSubWeaponTransform()
{
	if (!SubWeaponMeshComp || !SkeletalMeshComp)
	{
		return;
	}

	// 스켈레탈 메시가 로드되었는지 확인
	if (!SkeletalMeshComp->GetSkeletalMesh())
	{
		return;
	}

	// 본 인덱스 찾기
	int32 BoneIndex = SkeletalMeshComp->GetBoneIndexByName(FName(SubWeaponBoneName));
	if (BoneIndex < 0)
	{
		return;
	}

	// 본의 월드 트랜스폼 가져오기
	FTransform BoneTransform = SkeletalMeshComp->GetBoneWorldTransform(BoneIndex);

	// 오프셋 적용
	FVector FinalLocation = BoneTransform.Translation +
		BoneTransform.Rotation.RotateVector(SubWeaponOffset);

	// 회전 오프셋 적용 (오일러 → 쿼터니언)
	FQuat RotOffset = FQuat::MakeFromEulerZYX(SubWeaponRotationOffset);
	FQuat FinalRotation = BoneTransform.Rotation * RotOffset;

	// 무기 트랜스폼 설정
	SubWeaponMeshComp->SetWorldLocation(FinalLocation);
	SubWeaponMeshComp->SetWorldRotation(FinalRotation);
}
