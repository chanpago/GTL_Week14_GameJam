#include "BodySetupCore.h"
#include "FKShapeElem.h"

struct FKSphereElem;
struct FKBoxElem;
struct FKSphylElem;
struct FKConvexElem;

struct FKAggregateGeom
{
    TArray<FKSphereElem> SphereElements;
    TArray<FKBoxElem>    BoxElements;
    TArray<FKSphylElem>  SphylElements;
    TArray<FKConvexElem> ConvexElements;

    void Clear();
};

class UBodySetup : public UBodySetupCore
{
public:
    FKAggregateGeom AggGeom;             // 이 Body가 가진 Primitive Collision 모음

    float Mass = 10.0f;                
    float LinearDamping = 0.01f;  // 선속도를 줄이는 속도에 비례한 마찰
    float AngularDamping = 0.05f; // 각속도를 줄이는 회전에 대한 공기저항/마찰

    // Material
    // TODO : 직렬화 + 구조 짜기
    class UPhysicalMaterial* PhysMaterial = nullptr;

    // Collision Setting
    bool bSimulatePhysics = true; // false일 시 Static
    bool bEnableGravity = true;   // 위가 false면 의미없음

    void AddSphere(const FKSphereElem& Elem);
    void AddBox(const FKBoxElem& Elem);
    void AddSphyl(const FKSphylElem& Elem);
};