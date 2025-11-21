#pragma once

class UParticleModule;
class UParticleModuleRequired;
class UParticleModuleSpawn;
class UParticleModuleTypeDataBase;

class UParticleLODLevel : public UObject
{
public:
    void RebuildModuleCaches();
    
public:
    UParticleModuleRequired* RequiredModule = nullptr;
    UParticleModuleSpawn* SpawnModule = nullptr;
    UParticleModuleTypeDataBase* TypeDataModule = nullptr;

    TArray<UParticleModule*> SpawnModules;
    TArray<UParticleModule*> UpdateModules;

    // (선택) 전체 접근용 캐시
    TArray<UParticleModule*> AllModulesCache;

    int32 LODIndex = 0;
    bool  bEnabled = true;

};