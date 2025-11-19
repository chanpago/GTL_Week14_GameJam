#include "pch.h"
#include "FBXSceneUtilities.h"

// 헬퍼: Scene에 축 변환이 필요한지 확인하고 조건부로 적용
// 변환이 적용되면 true 반환, Scene이 이미 언리얼 축 시스템이면 false 반환
bool FBXSceneUtilities::ConvertSceneAxisIfNeeded(FbxScene* Scene)
{
	if (!Scene)
		return false;

	FbxAxisSystem UnrealImportAxis(FbxAxisSystem::eZAxis, FbxAxisSystem::eParityEven, FbxAxisSystem::eLeftHanded);
	FbxAxisSystem SourceSetup = Scene->GetGlobalSettings().GetAxisSystem();

	// Scene이 이미 언리얼 축 시스템인지 확인
	if (SourceSetup == UnrealImportAxis)
	{
		UE_LOG("Scene is already in Unreal axis system, skipping DeepConvertScene");

		// 단위 변환은 여전히 확인 필요
		FbxSystemUnit SceneUnit = Scene->GetGlobalSettings().GetSystemUnit();
		double SceneScaleFactor = SceneUnit.GetScaleFactor();
		const double Tolerance = 1e-6;

		// 스케일 팩터가 1.0과 크게 다른 경우에만 변환 (아직 미터 단위가 아닌 경우)
		if ((SceneUnit != FbxSystemUnit::m) && (std::abs(SceneScaleFactor - 1.0) > Tolerance))
		{
			UE_LOG("Scene unit scale factor is %.6f, converting to meters", SceneScaleFactor);
			FbxSystemUnit::m.ConvertScene(Scene);
			UE_LOG("Converted scene unit to meters");
			return true;
		}
		else
		{
			UE_LOG("Scene is already in meters (scale factor: %.6f), skipping unit conversion", SceneScaleFactor);
			return false;
		}
	}

	// Scene에 축 변환이 필요함
	UE_LOG("Converting scene from source axis system to Unreal axis system");

	// 단위 시스템 변환
	FbxSystemUnit SceneUnit = Scene->GetGlobalSettings().GetSystemUnit();
	double SceneScaleFactor = SceneUnit.GetScaleFactor();
	const double Tolerance = 1e-6;

	// 스케일 팩터가 1.0과 크게 다른 경우에만 변환
	if ((SceneUnit != FbxSystemUnit::m) && (std::abs(SceneScaleFactor - 1.0) > Tolerance))
	{
		UE_LOG("Scene unit scale factor is %.6f, converting to meters", SceneScaleFactor);
		FbxSystemUnit::m.ConvertScene(Scene);
		UE_LOG("Converted scene unit to meters");
	}
	else
	{
		UE_LOG("Scene already has scale factor 1.0, skipping unit conversion");
	}

	// 축 시스템 변환
	UnrealImportAxis.DeepConvertScene(Scene);
	UE_LOG("Applied DeepConvertScene to convert axis system");

	return true;
}

// 헬퍼: 노드가 스켈레톤 속성을 포함하는지 확인
bool FBXSceneUtilities::NodeContainsSkeleton(FbxNode* InNode)
{
	if (!InNode)
	{
		return false;
	}
	FString NodeName = InNode->GetName();
	int InNodeCount = InNode->GetNodeAttributeCount();
	for (int i = 0; i < InNodeCount; ++i)
	{
		if (FbxNodeAttribute* Attribute = InNode->GetNodeAttributeByIndex(i))
		{
			if (Attribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
			{
				return true;
			}
		}
	}
	return false;
}

// 헬퍼: 비-스켈레톤 부모 노드들(예: Armature)로부터 누적 보정 행렬 계산
// 부모 체인을 따라 올라가며 모든 비-스켈레톤 조상의 트랜스폼을 누적
FbxAMatrix FBXSceneUtilities::ComputeNonSkeletonParentCorrection(FbxNode* BoneNode, const TMap<const FbxNode*, FbxAMatrix>& NonSkeletonParentTransforms)
{
	FbxAMatrix AccumulatedCorrection;
	AccumulatedCorrection.SetIdentity();

	if (!BoneNode)
		return AccumulatedCorrection;

	// 부모 체인을 따라 올라감
	FbxNode* Parent = BoneNode->GetParent();
	while (Parent)
	{
		// 부모가 스켈레톤 노드이면 중지 (비-스켈레톤 조상만 관심)
		if (NodeContainsSkeleton(Parent))
		{
			break;
		}

		// 이 비-스켈레톤 부모에 저장된 트랜스폼이 있는지 확인
		const FbxAMatrix* ParentTransformPtr = NonSkeletonParentTransforms.Find(Parent);
		if (ParentTransformPtr)
		{
			// 부모의 트랜스폼을 누적 (로컬→글로벌 순서를 위해 오른쪽에 곱함)
			AccumulatedCorrection = (*ParentTransformPtr) * AccumulatedCorrection;
		}

		// 다음 부모로 이동
		Parent = Parent->GetParent();
	}

	return AccumulatedCorrection;
}

FbxAMatrix FBXSceneUtilities::GetBindPoseMatrix(FbxNode* Node)
{
	if (!Node)
	{
		return FbxAMatrix();
	}

	return Node->EvaluateGlobalTransform();
}

// 헬퍼: 노드의 자손 중에 스켈레톤 노드가 있는지 확인 (재귀적 검사)
// 자식이나 자손 중에 스켈레톤 속성을 포함하면 true 반환
bool FBXSceneUtilities::NodeContainsSkeletonInDescendants(FbxNode* InNode)
{
	if (!InNode)
		return false;

	// 모든 자식 확인
	for (int i = 0; i < InNode->GetChildCount(); ++i)
	{
		FbxNode* ChildNode = InNode->GetChild(i);

		// 자식 자체가 스켈레톤이면 true 반환
		if (NodeContainsSkeleton(ChildNode))
		{
			return true;
		}

		// 자식의 자손을 재귀적으로 확인
		if (NodeContainsSkeletonInDescendants(ChildNode))
		{
			return true;
		}
	}

	return false;
}

// 헬퍼: 노드의 직계 자식 중에만 스켈레톤 노드가 있는지 확인 (비재귀적)
// 직계 자식 중에 스켈레톤 속성을 포함하면 true 반환
bool FBXSceneUtilities::NodeContainsSkeletonInImmediateChildren(FbxNode* InNode)
{

	if (!InNode)
	{
		return false;
	}
	FString NodeName = InNode->GetName();
	// 직계 자식만 확인 (재귀 없음)

	int InNodeCount = InNode->GetChildCount();
	for (int i = 0; i < InNodeCount; ++i)
	{
		FbxNode* ChildNode = InNode->GetChild(i);
		FString ChildNodeName = ChildNode->GetName();
		// 자식 자체가 스켈레톤이면 true 반환
		int ChildAttributeCount = ChildNode->GetNodeAttributeCount();
		for (int j = 0; j < ChildAttributeCount; ++j)
		{
			if (FbxNodeAttribute* Attribute = ChildNode->GetNodeAttributeByIndex(j))
			{
				if (Attribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
				{
					return true;
				}
			}
		}
	}

	return false;
}

// 본 이름 정규화 (접두어 제거)
// "mixamorig:Hips" → "Hips", "Character1:Spine" → "Spine"
FString FBXSceneUtilities::NormalizeBoneName(const FString& BoneName)
{
	size_t ColonPos = BoneName.find(':');
	if (ColonPos != std::string::npos)
	{
		return BoneName.substr(ColonPos + 1);
	}
	return BoneName;
}
