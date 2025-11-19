#include "pch.h"
#include "FBXAnimationLoader.h"
#include "FBXAnimationCache.h"
#include "FBXSceneUtilities.h"
#include "Source/Runtime/Engine/Animation/AnimSequence.h"
#include "Source/Runtime/Engine/Animation/AnimDateModel.h"
#include "ObjectFactory.h"
#include "PathUtils.h"
#include <filesystem>

// 헬퍼 함수: 직계 자식 중에 스켈레톤이 있는 첫 번째 비-스켈레톤 노드를 재귀적으로 찾음
static FbxNode* FindNonSkeletonParentRecursive(FbxNode* Node)
{
	FString NodeName = Node->GetName();
	if (!Node)
	{
		return nullptr;
	}

	// 이 노드가 스켈레톤이 아니면서 직계 자식 중에 스켈레톤이 있는지 확인
	if (!FBXSceneUtilities::NodeContainsSkeleton(Node) && FBXSceneUtilities::NodeContainsSkeletonInImmediateChildren(Node))
	{
		FString FindArmatureNodeName = Node->GetName();
		return Node;
	}

	// 자식 노드들을 재귀적으로 탐색
	int ChildCount = Node->GetChildCount();
	for (int i = 0; i < ChildCount; ++i)
	{
		FbxNode* FoundNode = FindNonSkeletonParentRecursive(Node->GetChild(i));
		if (FoundNode)
		{
			return FoundNode;
		}
	}

	return nullptr;
}

void FBXAnimationLoader::ProcessAnimations(FbxScene* Scene, const FSkeletalMeshData& MeshData, const FString& FilePath, TArray<UAnimSequence*>& OutAnimations)
{
	// 첫 번째 비-스켈레톤 컨테이너 노드를 찾아서 보정용 트랜스폼 추출
	// 기존의 "Armature" 하드코딩 로직을 일반화하여 모든 비-스켈레톤 부모 노드에 대응 (예: CactusPA, Armature 등)
	FbxAMatrix ArmatureTransform;
	ArmatureTransform.SetIdentity();  // 기본값: 항등 행렬
	FbxNode* RootNode = Scene->GetRootNode();

	if (RootNode)
	{
		// 루트부터 재귀적으로 탐색
		FbxNode* NonSkeletonParentNode = FindNonSkeletonParentRecursive(RootNode);

		if (NonSkeletonParentNode)
		{
			// 비-스켈레톤 부모 노드를 찾음 - 트랜스폼 추출
			ArmatureTransform = NonSkeletonParentNode->EvaluateLocalTransform();

			FbxVector4 ArmatureScale = ArmatureTransform.GetS();
			FString NodeName = NonSkeletonParentNode->GetName();
			UE_LOG("Found Armature '%s' with Scale: (%.2f, %.2f, %.2f)",
				NodeName.c_str(), ArmatureScale[0], ArmatureScale[1], ArmatureScale[2]);
		}
		else
		{
			UE_LOG("No non-skeleton container found, skeleton directly under root (Mixamo-style FBX)");
		}
	}

	// 중요: 애니메이션 스택 개수 가져오기
	// FBX에 애니메이션이 있는지 확인
	int32 AnimStackCount = Scene->GetSrcObjectCount<FbxAnimStack>();

	for (int32 StackIndex = 0; StackIndex < AnimStackCount; ++StackIndex)
	{
		// 애니메이션 스택 가져오기
		FbxAnimStack* AnimStack = Scene->GetSrcObject<FbxAnimStack>(StackIndex);
		if (!AnimStack)
		{
			continue;
		}

		FbxAnimLayer* AnimLayer = AnimStack->GetMember<FbxAnimLayer>(0);
		if (!AnimLayer)
		{
			continue;
		}

		Scene->SetCurrentAnimationStack(AnimStack);
		FString AnimStackName = AnimStack->GetName();

		// 시간 범위 가져오기
		FbxTimeSpan TimeSpan = AnimStack->GetLocalTimeSpan();
		FbxTime StartTime = TimeSpan.GetStart();
		FbxTime StopTime = TimeSpan.GetStop();
		FbxTime Duration = StopTime - StartTime;

		// Scene의 프레임레이트 가져오기
		FbxTime::EMode TimeMode = Scene->GetGlobalSettings().GetTimeMode();
		float FrameRate = static_cast<float>(FbxTime::GetFrameRate(TimeMode));
		double PlayLength = Duration.GetSecondDouble();

		// 전체 프레임 수 계산 (KraftonGTL 방식)
		FbxLongLong FrameCount = Duration.GetFrameCount(TimeMode);
		int32 NumFrames = static_cast<int32>(FrameCount) + 1;  // 0부터 FrameCount까지 포함

		// 애니메이션을 저장할 AnimSequence 생성
		UAnimSequence* AnimSequence = NewObject<UAnimSequence>();
		AnimSequence->ObjectName = FName(AnimStackName);

		// Get DataModel
		UAnimDataModel* DataModel = AnimSequence->GetDataModel();
		if (!DataModel)
		{
			continue;
		}

		DataModel->SetFrameRate(static_cast<int32>(FrameRate));
		DataModel->SetPlayLength(static_cast<float>(PlayLength));
		DataModel->SetNumberOfFrames(NumFrames);
		DataModel->SetNumberOfKeys(NumFrames);

		// 본 노드 맵 구축
		FbxNode* RootNode = Scene->GetRootNode();
		TMap<FName, FbxNode*> BoneNodeMap;
		BuildBoneNodeMap(RootNode, BoneNodeMap);

		UE_LOG("ProcessAnimations: AnimStack='%s', BoneNodeMap size=%d, Skeleton bones=%d",
			AnimStackName.c_str(), BoneNodeMap.Num(), MeshData.Skeleton.Bones.Num());

		// 각 본에 대해 애니메이션 추출
		int32 ExtractedBones = 0;
		for (const FBone& Bone : MeshData.Skeleton.Bones)
		{
			FName BoneName = Bone.Name;
			if (!BoneNodeMap.Contains(BoneName))
			{
				static int LogCount = 0;
				if (LogCount++ < 5)
				{
					UE_LOG("Bone '%s' not found in BoneNodeMap", BoneName);
				}
				continue;
			}

			FbxNode* BoneNode = BoneNodeMap[BoneName];
			if (!BoneNode)
			{
				continue;
			}

			// 본 트랙 추가
			int32 TrackIndex = DataModel->AddBoneTrack(BoneName);
			if (TrackIndex == INDEX_NONE)
			{
				continue;
			}

			TArray<FVector> Positions;
			TArray<FQuat> Rotations;
			TArray<FVector> Scales;

			// 루트 본인지 확인
			bool bIsRootBone = (Bone.ParentIndex == -1);

			// 애니메이션 데이터 추출 (프레임 기반)
			ExtractBoneAnimation(BoneNode, AnimLayer, StartTime, FrameCount, TimeMode, Positions, Rotations, Scales, ArmatureTransform, bIsRootBone);

			// 데이터 모델에 키 설정
			DataModel->SetBoneTrackKeys(BoneName, Positions, Rotations, Scales);
			ExtractedBones++;
		}

		UE_LOG("Extracted animation data for %d bones", ExtractedBones);

		// 호환성 검사를 위해 본 이름 저장
		TArray<FName> BoneNames;
		for (const FBone& Bone : MeshData.Skeleton.Bones)
		{
			BoneNames.Add(Bone.Name);
		}
		AnimSequence->SetBoneNames(BoneNames);

		// ResourceManager에 애니메이션 등록
		FString AnimKey = FilePath + "_" + AnimStackName;
		RESOURCE.Add<UAnimSequence>(AnimKey, AnimSequence);

		OutAnimations.Add(AnimSequence);
		UE_LOG("Extracted animation: %s (Duration: %.2fs, Frames: %d, Bones: %d) -> Key: %s",
			AnimStackName.c_str(), static_cast<float>(PlayLength), NumFrames, BoneNames.Num(), AnimKey.c_str());
	}

#ifdef USE_OBJ_CACHE
	// 애니메이션을 캐시에 저장
	if (OutAnimations.Num() > 0)
	{
		FString NormalizedPath = NormalizePath(FilePath);
		FString CacheBasePath = ConvertDataPathToCachePath(NormalizedPath);
		FString AnimCacheDir = CacheBasePath + "_Anims";

		// 캐시 디렉토리 생성
		std::filesystem::create_directories(AnimCacheDir);

		for (int32 i = 0; i < OutAnimations.Num(); ++i)
		{
			FString AnimStackName = OutAnimations[i]->ObjectName.ToString();
			FString SanitizedName = AnimStackName;
			for (char& ch : SanitizedName)
			{
				if (ch == '|' || ch == ':' || ch == '*' || ch == '?' || ch == '"' || ch == '<' || ch == '>' || ch == '/' || ch == '\\')
				{
					ch = '_';
				}
			}
			FString AnimCachePath = AnimCacheDir + "/" + SanitizedName + ".anim.bin";

			if (FBXAnimationCache::SaveAnimationToCache(OutAnimations[i], AnimCachePath))
			{
				UE_LOG("Saved animation to cache: %s", AnimCachePath.c_str());
			}
			else
			{
				UE_LOG("Failed to save animation to cache: %s", AnimCachePath.c_str());
			}
		}

		UE_LOG("Saved %d animations to cache directory: %s", OutAnimations.Num(), AnimCacheDir.c_str());
	}
#endif
}

void FBXAnimationLoader::ExtractBoneAnimation(FbxNode* BoneNode, FbxAnimLayer* AnimLayer, FbxTime StartTime, FbxLongLong FrameCount, FbxTime::EMode TimeMode, TArray<FVector>& OutPositions, TArray<FQuat>& OutRotations, TArray<FVector>& OutScales, const FbxAMatrix& ArmatureTransform, bool bIsRootBone)
{
	if (!BoneNode || !AnimLayer || FrameCount < 0)
	{
		UE_LOG("ExtractBoneAnimation failed: BoneNode=%p, AnimLayer=%p, FrameCount=%lld", BoneNode, AnimLayer, FrameCount);
		return;
	}

	// 부모 노드 가져오기
	FbxNode* ParentNode = BoneNode->GetParent();

	// 처음부터 끝까지 모든 프레임을 샘플링 (KraftonGTL 방식)
	for (FbxLongLong Frame = 0; Frame <= FrameCount; Frame++)
	{
		FbxTime CurrentTime;
		CurrentTime.SetFrame(StartTime.GetFrameCount(TimeMode) + Frame, TimeMode);

		// Global Transform에서 Local Transform 계산 (ConvertScene 적용된 값 사용)
		FbxAMatrix GlobalTransform = BoneNode->EvaluateGlobalTransform(CurrentTime);
		FbxAMatrix LocalTransform;

		if (ParentNode)
		{
			FbxAMatrix ParentGlobalTransform = ParentNode->EvaluateGlobalTransform(CurrentTime);
			LocalTransform = ParentGlobalTransform.Inverse() * GlobalTransform;
		}
		else
		{
			LocalTransform = GlobalTransform;
		}

		// 루트 본에 Armature Transform 적용 (Blender FBX 지원)
		// Armature가 없으면 항등 행렬이므로 영향 없음
		if (bIsRootBone)
		{
			LocalTransform = ArmatureTransform * LocalTransform;
		}

		// 행렬에서 T, R, S 추출
		FbxVector4 Translation = LocalTransform.GetT();
		FbxQuaternion Rotation = LocalTransform.GetQ();
		FbxVector4 Scale = LocalTransform.GetS();

		// 트랙에 추가
		OutPositions.Add(FVector(
			static_cast<float>(Translation[0]),
			static_cast<float>(Translation[1]),
			static_cast<float>(Translation[2])
		));
		OutRotations.Add(FQuat(
			static_cast<float>(Rotation[0]),
			static_cast<float>(Rotation[1]),
			static_cast<float>(Rotation[2]),
			static_cast<float>(Rotation[3])
		));
		OutScales.Add(FVector(
			static_cast<float>(Scale[0]),
			static_cast<float>(Scale[1]),
			static_cast<float>(Scale[2])
		));
	}
}

bool FBXAnimationLoader::NodeHasAnimation(FbxNode* Node, FbxAnimLayer* AnimLayer)
{
	if (!Node || !AnimLayer)
	{
		return false;
	}

	// 위치 애니메이션 커브 확인
	FbxAnimCurve* TranslationX = Node->LclTranslation.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
	FbxAnimCurve* TranslationY = Node->LclTranslation.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
	FbxAnimCurve* TranslationZ = Node->LclTranslation.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);

	// 회전 애니메이션 커브 확인
	FbxAnimCurve* RotationX = Node->LclRotation.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
	FbxAnimCurve* RotationY = Node->LclRotation.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
	FbxAnimCurve* RotationZ = Node->LclRotation.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);

	// 스케일 애니메이션 커브 확인
	FbxAnimCurve* ScaleX = Node->LclScaling.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_X);
	FbxAnimCurve* ScaleY = Node->LclScaling.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_Y);
	FbxAnimCurve* ScaleZ = Node->LclScaling.GetCurve(AnimLayer, FBXSDK_CURVENODE_COMPONENT_Z);

	return (TranslationX || TranslationY || TranslationZ ||
		RotationX || RotationY || RotationZ ||
		ScaleX || ScaleY || ScaleZ);
}

void FBXAnimationLoader::BuildBoneNodeMap(FbxNode* RootNode, TMap<FName, FbxNode*>& OutBoneNodeMap)
{
	if (!RootNode)
	{
		return;
	}

	// 이 노드가 본인지 확인
	for (int32 i = 0; i < RootNode->GetNodeAttributeCount(); ++i)
	{
		FbxNodeAttribute* Attribute = RootNode->GetNodeAttributeByIndex(i);
		if (Attribute && Attribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
		{
			// 본 이름 정규화 (접두어 제거: "mixamorig:Hips" → "Hips")
			FName BoneName(FBXSceneUtilities::NormalizeBoneName(FString(RootNode->GetName())));
			OutBoneNodeMap.Add(BoneName, RootNode);
			break;
		}
	}

	// 자식 노드들을 재귀적으로 처리
	for (int32 i = 0; i < RootNode->GetChildCount(); ++i)
	{
		BuildBoneNodeMap(RootNode->GetChild(i), OutBoneNodeMap);
	}
}
