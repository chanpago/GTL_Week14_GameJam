#pragma once
#include "fbxsdk.h"
#include "SkeletalMesh.h"

class FBXSceneUtilities
{
public:
	// Scene에 축 변환이 필요한지 확인하고 조건부로 적용
	// 변환이 적용되면 true 반환, Scene이 이미 언리얼 축 시스템이면 false 반환
	static bool ConvertSceneAxisIfNeeded(FbxScene* Scene);

	// 노드가 스켈레톤 속성을 포함하는지 확인
	static bool NodeContainsSkeleton(FbxNode* InNode);

	// 노드의 자손 중에 스켈레톤 노드가 있는지 확인 (재귀적 검사)
	// 컨테이너 노드(예: Armature, CactusPA)를 관련 없는 노드(예: Mesh, Camera)와 구분하는 데 사용
	static bool NodeContainsSkeletonInDescendants(FbxNode* InNode);

	// 노드의 직계 자식 중에만 스켈레톤 노드가 있는지 확인 (비재귀적)
	// 직접 스켈레톤 컨테이너 노드를 찾는 데 사용
	static bool NodeContainsSkeletonInImmediateChildren(FbxNode* InNode);

	// 비-스켈레톤 부모 노드들(예: Armature)로부터 누적 보정 행렬 계산
	// 부모 체인을 따라 올라가며 모든 비-스켈레톤 조상의 트랜스폼을 누적
	static FbxAMatrix ComputeNonSkeletonParentCorrection(FbxNode* BoneNode, const TMap<const FbxNode*, FbxAMatrix>& NonSkeletonParentTransforms);

	// 노드의 바인드 포즈 행렬 가져오기
	static FbxAMatrix GetBindPoseMatrix(FbxNode* Node);

	// 본 이름 정규화 (접두어 제거)
	// "mixamorig:Hips" → "Hips", "Character1:Spine" → "Spine"
	static FString NormalizeBoneName(const FString& BoneName);
};
