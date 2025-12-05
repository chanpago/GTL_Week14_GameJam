#pragma once

#include "CombatTypes.h"

// ============================================================================
// IDamageable - 데미지를 받을 수 있는 모든 액터가 구현해야 하는 인터페이스
// ============================================================================
// 사용법:
//   class APlayerCharacter : public ACharacter, public IDamageable
//   class AEnemyBase : public ACharacter, public IDamageable
// ============================================================================

class IDamageable
{
public:
    virtual ~IDamageable() = default;

    // ========================================================================
    // 필수 구현 함수들
    // ========================================================================

    /**
     * 데미지를 받습니다.
     * @param DamageInfo 데미지 정보
     * @return 실제로 적용된 데미지 양
     */
    virtual float TakeDamage(const FDamageInfo& DamageInfo) = 0;

    /**
     * 생존 여부를 반환합니다.
     */
    virtual bool IsAlive() const = 0;

    /**
     * 현재 피격 가능한 상태인지 반환합니다.
     * (무적, 회피 중 등이면 false)
     */
    virtual bool CanBeHit() const = 0;

    // ========================================================================
    // 방어 관련 (가드/패리 시스템 사용 시 구현)
    // ========================================================================

    /**
     * 현재 가드 중인지 반환합니다.
     */
    virtual bool IsBlocking() const { return false; }

    /**
     * 현재 패리 윈도우인지 반환합니다.
     * (가드 시작 직후 짧은 시간)
     */
    virtual bool IsParrying() const { return false; }

    // ========================================================================
    // 피격 반응
    // ========================================================================

    /**
     * 피격 반응을 실행합니다.
     * (경직 애니메이션, 넉백 등)
     * @param Reaction 적용할 반응 타입
     * @param DamageInfo 데미지 정보 (방향 등에 활용)
     */
    virtual void OnHitReaction(EHitReaction Reaction, const FDamageInfo& DamageInfo) = 0;

    /**
     * 사망 처리를 실행합니다.
     */
    virtual void OnDeath() = 0;
};

// ============================================================================
// 유틸리티: IDamageable 캐스팅 헬퍼
// ============================================================================

/**
 * AActor를 IDamageable로 캐스팅합니다.
 * @param Actor 캐스팅할 액터
 * @return IDamageable 포인터 (실패 시 nullptr)
 */
template<typename T>
IDamageable* GetDamageable(T* Actor)
{
    return dynamic_cast<IDamageable*>(Actor);
}
