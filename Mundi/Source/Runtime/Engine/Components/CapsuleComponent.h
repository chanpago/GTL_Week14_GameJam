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

private:
	void CreatePhysXActor();
	void DestroyPhysXActor();
	void UpdatePhysXActorTransform();

	physx::PxRigidDynamic* PhysXActor = nullptr;
};