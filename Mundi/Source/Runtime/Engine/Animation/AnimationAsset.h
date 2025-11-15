#pragma once
#include "ResourceBase.h"


/**
 * @brief 애니메이션 이셋의 추상 기반 클래스
 * 모든 종류의 애니메이션 데이터를 표현하는 인터페이스
 */


/**
 * @brief 제공하는 기능
 * 1. 스켈레톤 연결 : 애니메이션과 스켈레톤을 매칭
 * 2. 재생 시간 : GetPlayLength() - 애니메이션 길이
 * 3. 추출 컨텍스트 : FAnimExtractContext - 시간/루핑 정보 전달
 */


struct FAnimExtractContext
{
    FAnimExtractContext(double InCurrentTime = 0.0, bool InbLooping = false) : CurrentTime(InCurrentTime), bLooping(InbLooping)
    {
    }
    // 현재 재생 시간
    double CurrentTime;

    // 루프 여부
    bool bLooping;
};

class USkeleton;

class UAnimationAsset : public UResourceBase
{
    DECLARE_CLASS(UAnimationAsset, UResourceBase)

public:
    UAnimationAsset() = default;
    virtual ~UAnimationAsset() = default;


public:
    virtual float GetPlayLength() const
    {
        return 0.f;
    }

    void SetSkeleton(USkeleton* NewSkeleton)
    {
        Skeleton = NewSkeleton;
    }

    USkeleton* GetSkeleton() const
    {
        return Skeleton;
    }

private:
    // 애니메이션이 적용될 스켈레톤
    USkeleton* Skeleton = nullptr;
};
