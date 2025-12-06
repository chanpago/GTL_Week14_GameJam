#pragma once
#include "Controller.h"
#include "APlayerController.generated.h"

class UTargetingComponent;

class APlayerController : public AController
{
public:
	GENERATED_REFLECTION_BODY()

	APlayerController();
	virtual ~APlayerController() override;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	virtual void SetupInput();

	// ========================================================================
	// Targeting System
	// ========================================================================
	UTargetingComponent* GetTargetingComponent() const { return TargetingComponent; }

	/** Lock-on 상태 (애니메이션 그래프에서 사용) */
	bool bIsLockOn = false;

protected:
    void ProcessMovementInput(float DeltaTime);
    void ProcessRotationInput(float DeltaTime);
    void ProcessLockOnInput();
    void ProcessLockedMovement(float DeltaTime, const FVector& WorldMoveDir);

protected:
    bool bMouseLookEnabled = true;

    // Targeting Component (replaces old bIsLockOn/LockedYaw)
    UTargetingComponent* TargetingComponent = nullptr;

    // Smooth rotation speed when locked on
    float CharacterRotationSpeed = 10.0f;

private:
	float Sensitivity = 0.1;
};
