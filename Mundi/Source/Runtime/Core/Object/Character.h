#pragma once
#include "Pawn.h"
#include "ACharacter.generated.h"

class UCapsuleComponent;
class USkeletalMeshComponent;
class UCharacterMovementComponent;
 
UCLASS(DisplayName = "캐릭터", Description = "캐릭터 액터")
class ACharacter : public APawn
{ 
public:
	GENERATED_REFLECTION_BODY()

	ACharacter();
	virtual ~ACharacter() override;

	virtual void Tick(float DeltaSecond) override;
    virtual void BeginPlay() override;
    void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	void DuplicateSubObjects() override;

	// 캐릭터 고유 기능
	virtual void Jump();
	virtual void StopJumping();
	 
	UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }
	UCharacterMovementComponent* GetCharacterMovement() const { return CharacterMovement; }

	// APawn 인터페이스: 파생 클래스의 MovementComponent를 노출
	virtual UPawnMovementComponent* GetMovementComponent() const override { return reinterpret_cast<UPawnMovementComponent*>(CharacterMovement); }

	//APawn에서 정의 됨
	USkeletalMeshComponent* GetMesh() const { return SkeletalMeshComp; }

	// ========================================================================
	// 무기 어태치 시스템
	// ========================================================================

	/** 무기 메시 컴포넌트 (컴포넌트는 별도 직렬화됨) */
	UStaticMeshComponent* WeaponMeshComp = nullptr;

	/** 무기 충돌 컴포넌트 (칼날 히트박스) */
	UCapsuleComponent* WeaponCollider = nullptr;

	/** 무기가 부착될 본 이름 */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	FString WeaponBoneName = "hand_r";

	/** 무기 오프셋 (본 기준 로컬) */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	FVector WeaponOffset = FVector::Zero();

	/** 무기 회전 오프셋 (본 기준 로컬, 오일러 각도) */
	UPROPERTY(EditAnywhere, Category = "Weapon")
	FVector WeaponRotationOffset = FVector::Zero();

	/** 무기 위치를 본에 맞춰 업데이트 */
	void UpdateWeaponTransform();

	/** 무기 충돌 활성화/비활성화 */
	void EnableWeaponCollision(bool bEnable);

	/** 무기 충돌 델리게이트 */
	DECLARE_DELEGATE(OnWeaponHit, AActor* /*HitActor*/, const FVector& /*HitLocation*/);

protected:
	/** 무기 충돌 시 호출 */
	void OnWeaponOverlap(AActor* OtherActor, const FVector& HitLocation);

    UCapsuleComponent* CapsuleComponent;
    UCharacterMovementComponent* CharacterMovement;

    /** 초기화 대기 프레임 카운터 */
    int32 InitFrameCounter = 0;
};
