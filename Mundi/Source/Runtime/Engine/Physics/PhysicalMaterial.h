#pragma once
class UPhysicalMaterial : public UObject
{
public:
    float StaticFriction;
    float DynamicFriction;
    float Restitution;
    float Density;
};