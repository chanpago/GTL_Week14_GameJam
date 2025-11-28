#include "pch.h"
#include "PhysicsAsset.h"
#include "BodySetup.h"

void UPhysicsAsset::BuildBodySetupIndexMap()
{
	BodySetupIndexMap.Empty();

	for (int32 Index = 0; Index < BodySetups.Num(); ++Index)
	{
		if (BodySetups[Index])
		{
			BodySetupIndexMap.Add(BodySetups[Index]->BoneName, Index);
		}
	}
}

int32 UPhysicsAsset::FindBodyIndex(FName BodyName) const
{
	if (const int32* Found = BodySetupIndexMap.Find(BodyName))
	{
		return *Found;
	}
	return INDEX_NONE;
}

UBodySetup* UPhysicsAsset::FindBodySetup(FName BodyName) const
{
	const int32 Index = FindBodyIndex(BodyName);
	return (Index != INDEX_NONE) ? BodySetups[Index] : nullptr;
}

int32 UPhysicsAsset::FindConstraintIndex(FName BodyA, FName BodyB) const
{
	return int32();
}
