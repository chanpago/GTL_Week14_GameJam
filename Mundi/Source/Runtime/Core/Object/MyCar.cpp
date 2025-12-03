#include "pch.h"
#include "MyCar.h"
#include "SkeletalMeshComponent.h"
#include "World.h"
#include "Source/Runtime/Engine/Physics/PhysScene.h"
#include "Source/Runtime/InputCore/InputManager.h"
#include <PxPhysicsAPI.h>

// PhysX Vehicle SDK includes
#include "vehicle/PxVehicleSDK.h"
#include "vehicle/PxVehicleUtil.h"
#include "vehicle/PxVehicleDrive4W.h"

using namespace physx;

// Helper function to create vehicle wheel data
static void SetupWheelsSimulationData(
    float WheelMass, float WheelRadius, float WheelWidth, float WheelMOI,
    const FVector& ChassisCMOffset, int32 NumWheels, PxVehicleWheelsSimData* WheelsSimData)
{
    // Setup wheels
    PxVec3 WheelCentreOffsets[PX_MAX_NB_WHEELS];
    
    // Front wheels
    WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eFRONT_LEFT] = 
        PxVec3(-1.0f, 0.0f, 1.5f);
    WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT] = 
        PxVec3(1.0f, 0.0f, 1.5f);
    
    // Rear wheels
    WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eREAR_LEFT] = 
        PxVec3(-1.0f, 0.0f, -1.5f);
    WheelCentreOffsets[PxVehicleDrive4WWheelOrder::eREAR_RIGHT] = 
        PxVec3(1.0f, 0.0f, -1.5f);

    // Setup wheel simulation data
    for (PxU32 i = 0; i < NumWheels; i++)
    {
        WheelsSimData->setWheelData(i, PxVehicleWheelData());
        WheelsSimData->setTireData(i, PxVehicleTireData());
        WheelsSimData->setSuspensionData(i, PxVehicleSuspensionData());
        WheelsSimData->setWheelShapeMapping(i, i);
        
        // Set wheel center offset
        WheelsSimData->setSuspTravelDirection(i, PxVec3(0, -1, 0));
        WheelsSimData->setWheelCentreOffset(i, WheelCentreOffsets[i]);
        
        // Set suspension force application point offset
        WheelsSimData->setSuspForceAppPointOffset(i, PxVec3(0, 0, 0));
        
        // Set tire force application point offset
        WheelsSimData->setTireForceAppPointOffset(i, PxVec3(0, 0, 0));
    }
}

AMyCar::AMyCar()
{
    // Create skeletal mesh component for the car
    VehicleMesh = CreateDefaultSubobject<USkeletalMeshComponent>("VehicleMesh");
    SetRootComponent(VehicleMesh);
    
    // Initialize wheel query results
    WheelQueryResults.SetNum(PX_MAX_NB_WHEELS);
}

AMyCar::~AMyCar()
{
    CleanupVehiclePhysics();
}

void AMyCar::BeginPlay()
{
    Super::BeginPlay();
    
    // Initialize PhysX vehicle
    InitializeVehiclePhysics();
    
    UE_LOG("[MyCarComponent] Vehicle initialized successfully");
}

void AMyCar::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (!VehicleDrive4W)
        return;
    
    // Process keyboard input
    ProcessKeyboardInput(DeltaTime);
    
    // Update vehicle physics
    UpdateVehiclePhysics(DeltaTime);
}

void AMyCar::EndPlay()
{
    CleanupVehiclePhysics();
    Super::EndPlay();
}

void AMyCar::InitializeVehiclePhysics()
{
    UWorld* World = GetWorld();
    if (!World || !World->GetPhysScene())
    {
        UE_LOG("[MyCarComponent] Failed to initialize: No PhysScene");
        return;
    }
    
    FPhysScene* PhysScene = World->GetPhysScene();
    PxScene* PxScenePtr = PhysScene->GetScene();
    PxPhysics* Physics = PhysScene->GetPhysics();
    
    if (!PxScenePtr || !Physics)
    {
        UE_LOG("[MyCarComponent] Failed to initialize: Invalid PhysX scene");
        return;
    }
    
    // Initialize PhysX Vehicle SDK
    if (!PxInitVehicleSDK(*Physics))
    {
        UE_LOG("[MyCarComponent] Failed to initialize PhysX Vehicle SDK");
        return;
    }
    
    // Set basis vectors (Z-up coordinate system)
    //PxVehicleSetBasisVectors(PxVec3(0, 1, 0), PxVec3(0, 0, 1));
    PxVehicleSetBasisVectors(PxVec3(0, 0, 1), PxVec3(1, 0, 0));
    
    // Set update mode
    PxVehicleSetUpdateMode(PxVehicleUpdateMode::eVELOCITY_CHANGE);
    
    // Create the vehicle
    CreateVehicle4W();
    
    UE_LOG("[MyCarComponent] PhysX Vehicle SDK initialized");
}

void AMyCar::CreateVehicle4W()
{
    UWorld* World = GetWorld();
    if (!World || !World->GetPhysScene())
        return;
    
    FPhysScene* PhysScene = World->GetPhysScene();
    PxPhysics* Physics = PhysScene->GetPhysics();
    PxScene* PxScenePtr = PhysScene->GetScene();
    PxMaterial* Material = PhysScene->GetDefaultMaterial();
    
    if (!Physics || !PxScenePtr || !Material)
        return;
    
    // Calculate MOI for chassis and wheels
    const PxVec3 ChassisHalfExtents(ChassisDimensions.X * 0.5f, 
                                    ChassisDimensions.Y * 0.5f, 
                                    ChassisDimensions.Z * 0.5f);
    
    const PxVec3 ChassisMOI(
        (ChassisHalfExtents.y * ChassisHalfExtents.y + ChassisHalfExtents.z * ChassisHalfExtents.z) * ChassisMass / 12.0f,
        (ChassisHalfExtents.x * ChassisHalfExtents.x + ChassisHalfExtents.z * ChassisHalfExtents.z) * 0.8f * ChassisMass / 12.0f,
        (ChassisHalfExtents.x * ChassisHalfExtents.x + ChassisHalfExtents.y * ChassisHalfExtents.y) * ChassisMass / 12.0f
    );
    
    const PxVec3 ChassisCMOffset(0.0f, -ChassisDimensions.Y * 0.5f + 0.65f, 0.25f);
    const FVector ChassisCMOffsetFVector(ChassisCMOffset.x, ChassisCMOffset.y, ChassisCMOffset.z);
    
    // Create rigid body
    FTransform WorldTransform = GetActorTransform();
    PxTransform StartPose(
        PxVec3(WorldTransform.Translation.X, WorldTransform.Translation.Y, WorldTransform.Translation.Z),
        PxQuat(WorldTransform.Rotation.X, WorldTransform.Rotation.Y, 
               WorldTransform.Rotation.Z, WorldTransform.Rotation.W)
    );
    
    PxRigidDynamic* VehicleActor = Physics->createRigidDynamic(StartPose);
    
    if (!VehicleActor)
    {
        UE_LOG("[MyCarComponent] Failed to create vehicle actor");
        return;
    }
    
    // Create chassis shape
    PxShape* ChassisShape = PxRigidActorExt::createExclusiveShape(
        *VehicleActor,
        PxBoxGeometry(ChassisHalfExtents),
        *Material
    );
    
    if (!ChassisShape)
    {
        VehicleActor->release();
        UE_LOG("[MyCarComponent] Failed to create chassis shape");
        return;
    }
    
    // Create wheel shapes
    for (int32 i = 0; i < NumWheels; i++)
    {
        PxShape* WheelShape = PxRigidActorExt::createExclusiveShape(
            *VehicleActor,
            PxSphereGeometry(WheelRadius),
            *Material
        );
        
        if (!WheelShape)
        {
            VehicleActor->release();
            UE_LOG("[MyCarComponent] Failed to create wheel shape %d", i);
            return;
        }
    }
    
    // Set mass and inertia
    VehicleActor->setMass(ChassisMass);
    VehicleActor->setMassSpaceInertiaTensor(ChassisMOI);
    VehicleActor->setCMassLocalPose(PxTransform(ChassisCMOffset, PxQuat(PxIdentity)));
    
    // Create vehicle drive data
    PxVehicleWheelsSimData* WheelsSimData = PxVehicleWheelsSimData::allocate(NumWheels);
    
    const float WheelMass = 20.0f;
    const float WheelMOI = 0.5f * WheelMass * WheelRadius * WheelRadius;
    
    SetupWheelsSimulationData(WheelMass, WheelRadius, WheelWidth, WheelMOI, 
                               ChassisCMOffsetFVector, NumWheels, WheelsSimData);
    
    // Create drive simulation data
    PxVehicleDriveSimData4W DriveSimData;
    
    // Setup differential (4WD)
    PxVehicleDifferential4WData Diff;
    Diff.mType = PxVehicleDifferential4WData::eDIFF_TYPE_LS_4WD;
    DriveSimData.setDiffData(Diff);
    
    // Setup engine
    PxVehicleEngineData Engine;
    Engine.mPeakTorque = 500.0f;
    Engine.mMaxOmega = 600.0f;
    DriveSimData.setEngineData(Engine);
    
    // Setup gears
    PxVehicleGearsData Gears;
    Gears.mSwitchTime = 0.5f;
    DriveSimData.setGearsData(Gears);
    
    // Setup auto-box
    PxVehicleAutoBoxData AutoBox;
    DriveSimData.setAutoBoxData(AutoBox);
    
    // Create the vehicle
    VehicleDrive4W = PxVehicleDrive4W::allocate(NumWheels);
    VehicleDrive4W->setup(Physics, VehicleActor, *WheelsSimData, DriveSimData, NumWheels - 4);
    
    // Free simulation data
    WheelsSimData->free();
    
    // Add actor to scene
    PxScenePtr->addActor(*VehicleActor);
    
    // Set to rest state and first gear
    VehicleDrive4W->setToRestState();
    VehicleDrive4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
    VehicleDrive4W->mDriveDynData.setUseAutoGears(true);
    
    UE_LOG("[MyCarComponent] Vehicle 4W created successfully");
}

void AMyCar::ProcessKeyboardInput(float DeltaTime)
{
    UInputManager& InputManager = UInputManager::GetInstance();
    
    // Reset inputs
    float TargetThrottle = 0.0f;
    float TargetBrake = 0.0f;
    float TargetSteer = 0.0f;
    float TargetHandbrake = 0.0f;
    
    // Arrow key controls
    if (InputManager.IsKeyDown(VK_UP))
    {
        TargetThrottle = 1.0f;
    }
    if (InputManager.IsKeyDown(VK_DOWN))
    {
        TargetBrake = 1.0f;
    }
    if (InputManager.IsKeyDown(VK_LEFT))
    {
        TargetSteer = -1.0f;
    }
    if (InputManager.IsKeyDown(VK_RIGHT))
    {
        TargetSteer = 1.0f;
    }
    if (InputManager.IsKeyDown(VK_SPACE))
    {
        TargetHandbrake = 1.0f;
    }
    
    // Smooth input transitions
    if (TargetThrottle > CurrentThrottleInput)
    {
        CurrentThrottleInput = FMath::Min(CurrentThrottleInput + ThrottleRiseRate * DeltaTime, TargetThrottle);
    }
    else
    {
        CurrentThrottleInput = FMath::Max(CurrentThrottleInput - ThrottleFallRate * DeltaTime, TargetThrottle);
    }
    
    if (TargetBrake > CurrentBrakeInput)
    {
        CurrentBrakeInput = FMath::Min(CurrentBrakeInput + ThrottleRiseRate * DeltaTime, TargetBrake);
    }
    else
    {
        CurrentBrakeInput = FMath::Max(CurrentBrakeInput - ThrottleFallRate * DeltaTime, TargetBrake);
    }
    
    if (TargetSteer > CurrentSteerInput)
    {
        CurrentSteerInput = FMath::Min(CurrentSteerInput + SteerRiseRate * DeltaTime, TargetSteer);
    }
    else if (TargetSteer < CurrentSteerInput)
    {
        CurrentSteerInput = FMath::Max(CurrentSteerInput - SteerRiseRate * DeltaTime, TargetSteer);
    }
    else
    {
        // Return to center
        if (CurrentSteerInput > 0.0f)
        {
            CurrentSteerInput = FMath::Max(CurrentSteerInput - SteerFallRate * DeltaTime, 0.0f);
        }
        else if (CurrentSteerInput < 0.0f)
        {
            CurrentSteerInput = FMath::Min(CurrentSteerInput + SteerFallRate * DeltaTime, 0.0f);
        }
    }
    
    CurrentHandbrakeInput = TargetHandbrake;
    
    // Apply to vehicle
    VehicleInput.AnalogAccel = CurrentThrottleInput;
    VehicleInput.AnalogBrake = CurrentBrakeInput;
    VehicleInput.AnalogSteer = CurrentSteerInput;
    VehicleInput.AnalogHandbrake = CurrentHandbrakeInput;
}

void AMyCar::UpdateVehiclePhysics(float DeltaTime)
{
    if (!VehicleDrive4W)
        return;
    
    UWorld* World = GetWorld();
    if (!World || !World->GetPhysScene())
        return;
    
    FPhysScene* PhysScene = World->GetPhysScene();
    PxScene* PxScenePtr = PhysScene->GetScene();
    
    if (!PxScenePtr)
        return;
    
    // Apply smoothed inputs
    VehicleDrive4W->mDriveDynData.setAnalogInput(PxVehicleDrive4WControl::eANALOG_INPUT_ACCEL, 
                                                  VehicleInput.AnalogAccel);
    VehicleDrive4W->mDriveDynData.setAnalogInput(PxVehicleDrive4WControl::eANALOG_INPUT_BRAKE, 
                                                  VehicleInput.AnalogBrake);
    VehicleDrive4W->mDriveDynData.setAnalogInput(PxVehicleDrive4WControl::eANALOG_INPUT_STEER_LEFT, 
                                                  FMath::Max(0.0f, -VehicleInput.AnalogSteer));
    VehicleDrive4W->mDriveDynData.setAnalogInput(PxVehicleDrive4WControl::eANALOG_INPUT_STEER_RIGHT, 
                                                  FMath::Max(0.0f, VehicleInput.AnalogSteer));
    VehicleDrive4W->mDriveDynData.setAnalogInput(PxVehicleDrive4WControl::eANALOG_INPUT_HANDBRAKE, 
                                                  VehicleInput.AnalogHandbrake);
    
    // Get vehicle transform from PhysX
    if (VehicleDrive4W->getRigidDynamicActor())
    {
        PxTransform PxTrans = VehicleDrive4W->getRigidDynamicActor()->getGlobalPose();
        
        FTransform NewTransform;
        NewTransform.Translation = FVector(PxTrans.p.x, PxTrans.p.y, PxTrans.p.z);
        NewTransform.Rotation = FQuat(PxTrans.q.x, PxTrans.q.y, PxTrans.q.z, PxTrans.q.w);
        NewTransform.Scale3D = FVector(1, 1, 1);
        
        SetActorTransform(NewTransform);
    }
}

void AMyCar::CleanupVehiclePhysics()
{
    if (VehicleDrive4W)
    {
        // Remove from scene
        if (VehicleDrive4W->getRigidDynamicActor())
        {
            UWorld* World = GetWorld();
            if (World && World->GetPhysScene())
            {
                FPhysScene* PhysScene = World->GetPhysScene();
                PxScene* PxScenePtr = PhysScene->GetScene();
                
                if (PxScenePtr)
                {
                    PxScenePtr->removeActor(*VehicleDrive4W->getRigidDynamicActor());
                }
            }
        }
        
        // Free vehicle
        VehicleDrive4W->free();
        VehicleDrive4W = nullptr;
    }
    
    // Cleanup PhysX Vehicle SDK
    PxCloseVehicleSDK();
    
    UE_LOG("[MyCarComponent] Vehicle physics cleaned up");
}

void AMyCar::ApplyThrottle(float Value)
{
    VehicleInput.AnalogAccel = FMath::Clamp(Value, 0.0f, 1.0f);
}

void AMyCar::ApplySteering(float Value)
{
    VehicleInput.AnalogSteer = FMath::Clamp(Value, -1.0f, 1.0f);
}

void AMyCar::ApplyBrake(float Value)
{
    VehicleInput.AnalogBrake = FMath::Clamp(Value, 0.0f, 1.0f);
}

void AMyCar::ApplyHandbrake(float Value)
{
    VehicleInput.AnalogHandbrake = FMath::Clamp(Value, 0.0f, 1.0f);
}
