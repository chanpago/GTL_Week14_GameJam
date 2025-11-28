#pragma once
#include "BodyInstance.h"



struct FConstraintLimitData;

struct FConstraintInstance
{

public:
    void InitD6(FPhysScene& World, const FTransform& ParentFrame, const FTransform& ChildFrame, const FConstraintLimitData& Limits);

    void Terminate(FPhysScene& World);
    
public:
    FBodyInstance* ParentBody   = nullptr;
    FBodyInstance* ChildBody    = nullptr;
    PxJoint*       Joint        = nullptr;
};
