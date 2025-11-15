#pragma once
#include "SkinnedMeshComponent.h"
#include "StatsOverlayD2D.h"
#include "UEContainer.h"

#define UPDATE_SKINNING_STATS(Meshes)\
FSkinningStatManager::GetInstance().GatherSkinnningStats(Meshes);

#define UPDATE_SKINNING_TYPE(bIsGPU)\
FSkinningStatManager::GetInstance().UpdateSkinningType(bIsGPU);    

struct FSkinningStats
{
    // 전체 스켈레탈 개수
    uint32 TotalSkeletals = 0;
    // 전제 bone 개수
    uint32 TotalBones = 0;
    // 전체 정점 개수
    uint32 TotalVertices = 0;

    FString SkinningType = "CPU";

    FSkinningStats() {};

    FSkinningStats(const FSkinningStats& other)
    {
        TotalSkeletals = other.TotalSkeletals;
        TotalBones = other.TotalBones;
        TotalVertices = other.TotalVertices;
        SkinningType = other.SkinningType;
    };

    void AddStats(const FSkinningStats& other)
    {
        TotalSkeletals += other.TotalSkeletals;
        TotalBones += other.TotalBones;
        TotalVertices += other.TotalVertices;
    }

    void Reset()
    {
        TotalSkeletals = 0;
        TotalBones = 0;
        TotalVertices = 0;
    }
};

class FSkinningStatManager
{
public:
    static FSkinningStatManager& GetInstance()
    {
        static FSkinningStatManager Instance;
        return Instance;
    }

    void UpdateStats(const FSkinningStats& InStats)
    {
        CurrentStats = InStats;
    }

    const FSkinningStats& GetStats() const
    {
        return CurrentStats;
    }

    void ResetStats()
    {
        CurrentStats.Reset();
    }

    void UpdateSkinningType(bool bEnableGPUSkinning)
    {
        CurrentStats.SkinningType = bEnableGPUSkinning ? "GPU" : "CPU";
    }

    void GatherSkinnningStats(TArray<UMeshComponent*>& Components)
    {
        if(Components.IsEmpty() || !UStatsOverlayD2D::Get().IsSkinningVisible())
        {
            return;
        }

        for (UMeshComponent* MeshComp : Components)
        {
            if (USkinnedMeshComponent* SkinnedComp = Cast<USkinnedMeshComponent>(MeshComp))
            {
                CurrentStats.TotalSkeletals++;
                if (USkeletalMesh* SkeletalMesh = SkinnedComp->GetSkeletalMesh())
                {
                    CurrentStats.TotalBones += SkeletalMesh->GetBoneCount();
                    CurrentStats.TotalVertices += SkeletalMesh->GetVertexCount();
                }
            }
        }
    }

private:
    FSkinningStatManager() = default;
    ~FSkinningStatManager() = default;
    FSkinningStatManager(const FSkinningStatManager&) = delete;
    FSkinningStatManager& operator=(const FSkinningStatManager&) = delete;

    FSkinningStats CurrentStats;
};