#pragma once
#include "PrimitiveComponent.h"
#include "Source/Runtime/Engine/Particle/ParticleEmitter.h"
#include "Source/Runtime/Engine/Particle/ParticleSystem.h"
#include "Source/Runtime/Engine/Particle/ParticleHelper.h"
#include "Source/Runtime/Engine/Particle/DynamicEmitterDataBase.h"
#include "UParticleSystemComponent.generated.h"

UCLASS(DisplayName = "파티클 컴포넌트", Description = "파티클을 생성하는 컴포넌트")
class UParticleSystemComponent : public UPrimitiveComponent
{
public: 
	GENERATED_REFLECTION_BODY()
	DECLARE_DELEGATE(OnParticleSystemFinished, UParticleSystemComponent*);

public:
	UParticleSystemComponent();
	~UParticleSystemComponent() override;

public:
	// 컴포넌트 초기화 & LifeCycle
	virtual void InitParticles();
	virtual void DestroyParticles();

	void BeginPlay() override;
	void EndPlay() override;
	void TickComponent(float DeltaTime) override;

	// ParticleSystem 활성화/비활성화 제어
	void ActivateSystem() { bAutoActivate = true; }
	void DeactivateSystem() { bAutoActivate = false; }
	void ResetAndActivate() { InitParticles(); ActivateSystem(); SetActive(true); }

	// Template accessor
	void SetTemplate(UParticleSystem* InTemplate) { Template = InTemplate; InitParticles(); }
	UParticleSystem* GetTemplate() const { return Template; }

	// 렌더링을 위한 MeshBatch 수집 함수
	void CollectMeshBatches(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View) override;

	void DuplicateSubObjects() override;

private:
	// sprite, mesh 나눠 BuildBatch
	void BuildEmitterRenderData();
	void BuildSpriteParticleBatch(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View);
	void BuildMeshParticleBatch(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View);
	UMaterialInterface* ResolveEmitterMaterial(const FDynamicEmitterDataBase& DynData) const;

	// Resource 관리
	bool EnsureParticleBuffers(uint32 ParticleCapacity);
	void ReleaseParticleBuffers();
	void ClearEmitterRenderData();
	
private:	
	UPROPERTY(EditAnywhere, Category = "Particle", DisplayName = "파티클 시스템")
	UParticleSystem* Template = nullptr;

	// Runtime Data
	TArray<FParticleEmitterInstance*> EmitterInstances;
	TArray<FDynamicEmitterDataBase*> EmitterRenderData; 	/** 렌더 스레드로 보낼 데이터 패킷들 */	
	
	// Render Resources
	ID3D11Buffer* ParticleVertexBuffer = nullptr;
	ID3D11Buffer* ParticleIndexBuffer = nullptr;
	uint32 ParticleVertexCapacity = 0;
	uint32 ParticleIndexCount = 0;
	UMaterialInterface* FallbackMaterial = nullptr;

	// Settings
	bool bAutoActivate = true;
	bool bAutoDestroy = false;
	int MaxDebugParticles = 10000;

	// Debug
	struct FDebugMeshParticleState
	{
		bool bEnabled = true;
		bool bInitialized = false;

		float  TimeSeconds = 0.f;

		FVector BaseLocation = FVector(0.f, 0.f, 5.f);
		FVector Velocity = FVector(0.f, 0.f, 5.f);
		FVector CurrentLocation = FVector::Zero();

		UStaticMesh* Mesh = nullptr;
		UMaterialInterface* Material = nullptr;
	};
};
