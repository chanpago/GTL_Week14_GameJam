#pragma once

#include "ShapeComponent.h"
#include "UCapsuleComponent.generated.h"

// PhysX 전방 선언
namespace physx
{
	class PxRigidDynamic;
}

UCLASS(DisplayName="캡슐 컴포넌트", Description="캡슐 모양 충돌 컴포넌트입니다")
class UCapsuleComponent : public UShapeComponent
{
public:

	GENERATED_REFLECTION_BODY();

	UCapsuleComponent();
	~UCapsuleComponent();

	void OnRegister(UWorld* World) override;
	void OnUnregister() override;
	void TickComponent(float DeltaSeconds) override;

	// Duplication
	virtual void DuplicateSubObjects() override;

	UPROPERTY(EditAnywhere, Category="CapsuleHalfHeight")
	float CapsuleHalfHeight;

	UPROPERTY(EditAnywhere, Category="CapsuleHalfHeight")
	float CapsuleRadius;

	void GetShape(FShape& Out) const override;

public:
	FAABB GetWorldAABB() const override;
	void RenderDebugVolume(class URenderer* Renderer) const override;

	// PVD 디버깅용 - PhysX에 Kinematic Actor로 등록
	UPROPERTY(EditAnywhere, Category="Physics")
	bool bRegisterToPhysX = true;

	// 트리거(오버랩) 충돌 활성화
	void EnableTriggerCollision(bool bEnable);
	bool IsTriggerEnabled() const { return bTriggerEnabled; }

	// 충돌 시 호출될 델리게이트
	DECLARE_DELEGATE(OnTriggerHit, class AActor* /*OtherActor*/, const FVector& /*HitLocation*/);

	// 수동으로 PhysX Actor 생성/파괴 (public)
	void CreatePhysXActor();
	void DestroyPhysXActor();

private:
	void UpdatePhysXActorTransform();
	void CheckTriggerOverlaps();

	physx::PxRigidDynamic* PhysXActor = nullptr;
	bool bTriggerEnabled = false;
	bool bOwnsPhysXActor = true;  // 복제본은 false - PhysXActor 해제 안함
	TArray<class AActor*> OverlappedActors;  // 중복 충돌 방지
};