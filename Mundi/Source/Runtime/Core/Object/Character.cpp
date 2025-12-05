#include "pch.h"
#include "Character.h"
#include "CapsuleComponent.h"
#include "SkeletalMeshComponent.h"
#include "CharacterMovementComponent.h" 
#include"StaticMeshComponent.h"
#include "ObjectMacros.h" 
ACharacter::ACharacter()
{
	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>("CapsuleComponent");
	//CapsuleComponent->SetSize();
    WeaponMeshComp = CreateDefaultSubobject<UStaticMeshComponent>("StaticMeshComponent");
	SetRootComponent(CapsuleComponent);

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
        SkeletalMeshComp = nullptr;

        for (UActorComponent* Comp : GetOwnedComponents())
        {
            if (auto* Cap = Cast<UCapsuleComponent>(Comp))
            {
                CapsuleComponent = Cap;
            }
            else if (auto* Move = Cast<UCharacterMovementComponent>(Comp))
            {
                CharacterMovement = Move;
            }
            else if (auto* Skel = Cast<USkeletalMeshComponent>(Comp))
            {
                SkeletalMeshComp = Skel;
            }
            else if (auto* Weapon = Cast<UStaticMeshComponent>(Comp))
            {
                WeaponMeshComp = Weapon;
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
    SkeletalMeshComp = nullptr;

    for (UActorComponent* Comp : GetOwnedComponents())
    {
        if (auto* Cap = Cast<UCapsuleComponent>(Comp))
        {
            CapsuleComponent = Cap;
        }
        else if (auto* Move = Cast<UCharacterMovementComponent>(Comp))
        {
            CharacterMovement = Move;
        }
        else if (auto* Skel = Cast<USkeletalMeshComponent>(Comp))
        {
            SkeletalMeshComp = Skel;
        }
        else if (auto* Weapon = Cast<UStaticMeshComponent>(Comp))
        {
            WeaponMeshComp = Weapon;
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
