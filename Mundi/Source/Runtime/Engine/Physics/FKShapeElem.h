#pragma once
struct FKShapeElem
{
};

struct FKSphereElem
{
    FVector Center = FVector::Zero();
    float   Radius = 50.0f;

    // 디버그 / 에디터용 색상, 아이디 등 필요하면 추가
};

struct FKBoxElem
{
    FVector Center = FVector::Zero();
    FVector Extents = FVector(50.0f, 50.0f, 50.0f); // half size
    FQuat   Rotation = FQuat::Identity();
};

struct FKSphylElem // Capsule
{
    FVector Center = FVector::Zero();
    float   Radius = 30.0f;
    float   HalfLength = 50.0f;
    FQuat   Rotation = FQuat::Identity();
};

// TODO : 추후 구현
//struct FKConvexElem
//{
//    // Convex Hull 데이터
//    TArray<FVector> Vertices;
//};