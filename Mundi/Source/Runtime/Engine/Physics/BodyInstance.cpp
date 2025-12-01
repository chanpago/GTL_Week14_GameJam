#include "pch.h"
#include "BodyInstance.h"
#include "BodySetup.h"
#include "PhysScene.h"          // FPhysScene 선언
#include "PhysicsTypes.h"       // ToPx / FromPx (FVector/FTransform <-> Px 타입 변환)


using namespace physx;

// -------------------------
// Dynamic Body 초기화
// -------------------------
void FBodyInstance::InitDynamic(FPhysScene& World, const FTransform& WorldTransform, float Mass, const FVector& Scale3D)
{
    // 이미 RigidActor가 존재하면 정리
    if (RigidActor)
    {
        Terminate(World);
    }

    PxPhysics*      Physics     = World.GetPhysics();
    PxScene*        Scene       = World.GetScene();
    PxMaterial*     Material    = World.GetDefaultMaterial();

    if (!Physics || !Scene || !Material)
    {
        return;
    }

    // 엔진 Transform(FTransform) -> PhysX Transform(PxTransform)
    PxTransform Pose = ToPx(WorldTransform);

    // Dynamic Body 생성
    PxRigidDynamic* DynamicActor = Physics->createRigidDynamic(Pose);
    if (!DynamicActor)
    {
        return;
    }

     // ★★★★★ 여기부터가 핵심: BodySetup->AggGeom 기반으로 Shape 생성
    if (BodySetup)
    {
        const FKAggregateGeom& Agg = BodySetup->AggGeom;

        // 스케일의 절대값 사용 (음수 스케일 대응)
        const FVector AbsScale(std::fabs(Scale3D.X), std::fabs(Scale3D.Y), std::fabs(Scale3D.Z));

        // 1) 스피어들
        for (const FKSphereElem& Sphere : Agg.SphereElements)
        {
            // 스피어 중심도 스케일 적용
            FVector ScaledCenter = Sphere.Center * Scale3D;
            FTransform LocalXform(ScaledCenter, FQuat::Identity(), FVector(1,1,1));
            PxTransform LocalPose = ToPx(LocalXform);

            // 균등 스케일이 아닌 경우 가장 큰 축 사용
            float MaxScale = std::max({AbsScale.X, AbsScale.Y, AbsScale.Z});
            PxSphereGeometry Geom(Sphere.Radius * MaxScale);
            PxShape* Shape = Physics->createShape(Geom, *Material);
            if (Shape)
            {
                Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
                Shape->setLocalPose(LocalPose);
                DynamicActor->attachShape(*Shape);
                Shape->release();
            }
        }

        // 2) 박스들
        for (const FKBoxElem& Box : Agg.BoxElements)
        {
            // 박스 중심도 스케일 적용
            FVector ScaledCenter = Box.Center * Scale3D;
            FTransform LocalXform(ScaledCenter, Box.Rotation, FVector(1,1,1));
            PxTransform LocalPose = ToPx(LocalXform);

            // 박스 Extents에 스케일 적용
            PxBoxGeometry Geom(
                Box.Extents.X * AbsScale.X,
                Box.Extents.Y * AbsScale.Y,
                Box.Extents.Z * AbsScale.Z
            );
            PxShape* Shape = Physics->createShape(Geom, *Material);
            if (Shape)
            {
                Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
                Shape->setLocalPose(LocalPose);
                DynamicActor->attachShape(*Shape);
                Shape->release();
            }
        }

        // 3) 캡슐(Capsule)들
        for (const FKCapsuleElem& Capsule : Agg.CapsuleElements)
        {
            // 캡슐 중심도 스케일 적용
            FVector ScaledCenter = Capsule.Center * Scale3D;

            // Z축 캡슐 → PhysX X축 캡슐로 보정
            FQuat ZToX = FQuat::FromAxisAngle(FVector(0, 1, 0), -XM_PIDIV2);
            FQuat PhysRot = Capsule.Rotation * ZToX;

            FTransform LocalXform(ScaledCenter, PhysRot, FVector(1,1,1));
            PxTransform LocalPose = ToPx(LocalXform);

            // 캡슐: 반지름은 XY 평균, 높이는 Z 스케일 적용
            float RadiusScale = (AbsScale.X + AbsScale.Y) * 0.5f;
            float HeightScale = AbsScale.Z;

            PxCapsuleGeometry Geom(Capsule.Radius * RadiusScale, Capsule.HalfLength * HeightScale);

            PxShape* Shape = Physics->createShape(Geom, *Material);
            if (Shape)
            {
                Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
                Shape->setLocalPose(LocalPose);
                DynamicActor->attachShape(*Shape);
                Shape->release();
            }
        }
    }
    else
    {
        // BodySetup이 없으면, 지금처럼 임시 박스 하나라도 붙여서 디버그용으로 사용
        PxBoxGeometry BoxGeom(0.5f, 0.5f, 0.5f);
        PxShape* Shape = Physics->createShape(BoxGeom, *Material);
        if (Shape)
        {
            Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
            DynamicActor->attachShape(*Shape);
            Shape->release();
        }
    }

    // 질량 / 관성 텐서 설정
    PxRigidBodyExt::updateMassAndInertia(*DynamicActor, Mass);

    // 씬에 등록
    Scene->addActor(*DynamicActor);

    // FBodyInstance와 연결
    RigidActor           = DynamicActor;
    RigidActor->userData = this; // 나중에 콜백에서 FBodyInstance로 되돌리기 용
    
}

// -------------------------
// Static Body 초기화
// -------------------------
void FBodyInstance::InitStatic(FPhysScene& World, const FTransform& WorldTransform, const FVector& Scale3D)
{
    // 이미 바디가 존재하면 정리
    if (RigidActor)
    {
        Terminate(World);
    }

    PxPhysics*   Physics   = World.GetPhysics();
    PxScene*     Scene     = World.GetScene();
    PxMaterial*  Material  = World.GetDefaultMaterial();

    if (!Physics || !Scene || !Material)
        return;

    PxTransform Pose = ToPx(WorldTransform);

    // Static 바디 생성
    PxRigidStatic* StaticActor = Physics->createRigidStatic(Pose);
    if (!StaticActor)
    {
        return;
    }
    
    // BodySetup->AggGeom 기반으로 Shape 생성
    if (BodySetup)
    {
        const FKAggregateGeom& Agg = BodySetup->AggGeom;

        // 스케일의 절대값 사용 (음수 스케일 대응)
        const FVector AbsScale(std::fabs(Scale3D.X), std::fabs(Scale3D.Y), std::fabs(Scale3D.Z));

        // 1) 스피어들
        for (const FKSphereElem& Sphere : Agg.SphereElements)
        {
            // 스피어 중심도 스케일 적용
            FVector ScaledCenter = Sphere.Center * Scale3D;
            FTransform LocalXform(ScaledCenter, FQuat::Identity(), FVector(1,1,1));
            PxTransform LocalPose = ToPx(LocalXform);

            // 균등 스케일이 아닌 경우 가장 큰 축 사용
            float MaxScale = std::max({AbsScale.X, AbsScale.Y, AbsScale.Z});
            PxSphereGeometry Geom(Sphere.Radius * MaxScale);
            PxShape* Shape = Physics->createShape(Geom, *Material);
            if (Shape)
            {
                Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
                Shape->setLocalPose(LocalPose);
                StaticActor->attachShape(*Shape);
                Shape->release();
            }
        }

        // 2) 박스들
        for (const FKBoxElem& Box : Agg.BoxElements)
        {
            // 박스 중심도 스케일 적용
            FVector ScaledCenter = Box.Center * Scale3D;
            FTransform LocalXform(ScaledCenter, Box.Rotation, FVector(1,1,1));
            PxTransform LocalPose = ToPx(LocalXform);

            // 박스 Extents에 스케일 적용
            PxBoxGeometry Geom(
                Box.Extents.X * AbsScale.X,
                Box.Extents.Y * AbsScale.Y,
                Box.Extents.Z * AbsScale.Z
            );
            PxShape* Shape = Physics->createShape(Geom, *Material);
            if (Shape)
            {
                Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
                Shape->setLocalPose(LocalPose);
                StaticActor->attachShape(*Shape);
                Shape->release();
            }
        }

        // 3) 캡슐(Capsule)들
        for (const FKCapsuleElem& Capsule : Agg.CapsuleElements)
        {
            // 캡슐 중심도 스케일 적용
            FVector ScaledCenter = Capsule.Center * Scale3D;

            // Z축 캡슐 → PhysX X축 캡슐로 보정
            FQuat ZToX = FQuat::FromAxisAngle(FVector(0, 1, 0), -XM_PIDIV2);
            FQuat PhysRot = Capsule.Rotation * ZToX;

            FTransform LocalXform(ScaledCenter, PhysRot, FVector(1,1,1));
            PxTransform LocalPose = ToPx(LocalXform);

            // 캡슐: 반지름은 XY 평균, 높이는 Z 스케일 적용
            float RadiusScale = (AbsScale.X + AbsScale.Y) * 0.5f;
            float HeightScale = AbsScale.Z;

            PxCapsuleGeometry Geom(Capsule.Radius * RadiusScale, Capsule.HalfLength * HeightScale);
            PxShape* Shape = Physics->createShape(Geom, *Material);
            if (Shape)
            {
                Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
                Shape->setLocalPose(LocalPose);
                StaticActor->attachShape(*Shape);
                Shape->release();
            }
        }
    }
    else
    {
        // BodySetup이 없으면 임시 박스 하나 생성
        PxBoxGeometry BoxGeom(0.5f, 0.5f, 0.5f);
        PxShape* Shape = Physics->createShape(BoxGeom, *Material);
        if (Shape)
        {
            Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
            StaticActor->attachShape(*Shape);
            Shape->release();
        }
    }

    Scene->addActor(*StaticActor);

    RigidActor           = StaticActor;
    RigidActor->userData = this;
}

void FBodyInstance::Terminate(FPhysScene& World)
{
    if (!RigidActor)
    {
        return;
    }

    
    PxScene* Scene = World.GetScene();
    if (Scene)
    {
        // 씬에서 제거
        Scene->removeActor(*RigidActor);
    }

    // Actor 자체 Release
    RigidActor->release();
    RigidActor = nullptr;
}

void FBodyInstance::AddForce(const FVector& Force)
{
    if (!RigidActor)
    {
        return;
    }
    
    // Dynamic인지 체크
    PxRigidDynamic* DynamicActor = RigidActor->is<PxRigidDynamic>();
    if (!DynamicActor)
    {
        return;
    }
    
    PxVec3 PxForce = ToPx(Force);
    DynamicActor->addForce(PxForce, PxForceMode::eFORCE);
}

FTransform FBodyInstance::GetWorldTransform() const
{
    if (!RigidActor)
    {
        return FTransform(); // 기본 생성자 = Identity라고 가정
    }

    PxTransform Pose = RigidActor->getGlobalPose();
    return FromPx(Pose);
}
