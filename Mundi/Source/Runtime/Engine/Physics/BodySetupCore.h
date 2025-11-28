#pragma once
class UBodySetupCore : public UObject
{
public:
	FName BoneName;

	// Collision Enabled, Channel Response 같은 공통 설정
	// 나중에 메타데이터도 설정 
};