#include "pch.h"
#include "SimulationEventCallback.h"
#include "PhysScene.h"
#include "BodyInstance.h"
#include "../Components/PrimitiveComponent.h"
#include "../Object/Actor.h"


FSimulationEventCallback::FSimulationEventCallback(FPhysScene* owner)
	: OwnerScene(owner)
{
}

void FSimulationEventCallback::onContact(const PxContactPairHeader& PairHeader, const PxContactPair* Pairs, PxU32 NumPairs)
{
	if (!OwnerScene)
	{
		return;
	}

	// 1. Get the low-level physx actors
	PxRigidActor* ActorA_px = PairHeader.actors[0];
	PxRigidActor* ActorB_px = PairHeader.actors[1];

	// 2. Retrieve the FBodyInstance from userData.
	FBodyInstance* BodyInstA = static_cast<FBodyInstance*>(ActorA_px->userData);
	FBodyInstance* BodyInstB = static_cast<FBodyInstance*>(ActorB_px->userData);

	if (BodyInstA && BodyInstB)
	{
		// 3. Get the component that owns the body instance.
		UPrimitiveComponent* PrimCompA = BodyInstA->OwnerComponent;
		UPrimitiveComponent* PrimCompB = BodyInstB->OwnerComponent;

		if (PrimCompA && PrimCompB)
		{
			// 4. Get the final game Actor that owns the component.
			AActor* OwnerA = PrimCompA->GetOwner();
			AActor* OwnerB = PrimCompB->GetOwner();

			if (OwnerA && OwnerB)
			{
				// 5. Populate the FContactHit struct with the correct actors.
				FContactHit ContactHit;
				ContactHit.ActorA = OwnerA;
				ContactHit.ActorB = OwnerB;
				// TODO: Fill Position, Normal, and Impulse from the PxContactPair data if needed.
				// For now, we are just notifying that a contact happened.

				// 6. Execute the delegate.
				// It's a single-cast delegate, so we use ExecuteIfBound.
				OwnerScene->OnContactDelegate.Broadcast(ContactHit);
			}
		}
	}
}
