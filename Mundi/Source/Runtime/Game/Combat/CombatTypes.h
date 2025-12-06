#pragma once

#include "Vector.h"

class AActor;

// ============================================================================
// 공격 타입
// ============================================================================
enum class EDamageType
{
    Light,      // 약공격 - 경직 짧음
    Heavy,      // 강공격 - 경직 김
    Special,    // 특수기 - 넉백
    Parried     // 패리당함 - 큰 경직
};

// ============================================================================
// 피격 반응 타입
// ============================================================================
enum class EHitReaction
{
    None,       // 반응 없음 (슈퍼아머)
    Flinch,     // 살짝 경직
    Stagger,    // 큰 경직
    Knockback,  // 넉백
    Knockdown   // 넘어짐
};

// ============================================================================
// 전투 상태 (플레이어/적 공용)
// ============================================================================
enum class ECombatState
{
    Idle,           // 대기
    Attacking,      // 공격 중
    Dodging,        // 회피 중
    Blocking,       // 가드 중
    Parrying,       // 패리 중 (패리 윈도우)
    Staggered,      // 경직 중
    Knockback,      // 넉백 중
    Dead            // 사망
};

// ============================================================================
// 데미지 정보 구조체 (공격자 → 피격자로 전달)
// ============================================================================
struct FDamageInfo
{
    // 공격 주체
    AActor* Instigator = nullptr;       // 공격한 액터 (플레이어 or 적)
    AActor* DamageCauser = nullptr;     // 데미지 원인 (무기, 투사체 등)

    // 데미지 수치
    float Damage = 0.f;                 // 데미지 양
    EDamageType DamageType = EDamageType::Light;
    EHitReaction HitReaction = EHitReaction::Flinch;

    // 위치/방향 정보
    FVector HitLocation = FVector();      // 맞은 위치 (월드)
    FVector HitDirection = FVector();     // 맞은 방향 (넉백용)

    // 추가 파라미터
    float StaggerDuration = 0.3f;       // 경직 시간
    float KnockbackForce = 0.f;         // 넉백 힘

    // 방어 관련
    bool bCanBeBlocked = true;          // 가드 가능 여부
    bool bCanBeParried = true;          // 패리 가능 여부

    // 기본 생성자
    FDamageInfo() = default;

    // 편의 생성자
    FDamageInfo(AActor* InInstigator, float InDamage, EDamageType InType = EDamageType::Light)
        : Instigator(InInstigator)
        , Damage(InDamage)
        , DamageType(InType)
    {
        // 타입에 따른 기본값 설정
        switch (InType)
        {
        case EDamageType::Light:
            HitReaction = EHitReaction::Flinch;
            StaggerDuration = 0.2f;
            break;
        case EDamageType::Heavy:
            HitReaction = EHitReaction::Stagger;
            StaggerDuration = 0.5f;
            break;
        case EDamageType::Special:
            HitReaction = EHitReaction::Knockback;
            StaggerDuration = 0.8f;
            KnockbackForce = 500.f;
            break;
        case EDamageType::Parried:
            HitReaction = EHitReaction::Stagger;
            StaggerDuration = 1.5f;
            break;
        }
    }
};

// ============================================================================
// 전투 히트 결과 (TakeDamage 반환값으로 사용 가능)
// ============================================================================
struct FCombatHitResult
{
    bool bWasHit = false;           // 실제로 맞았는지
    bool bWasBlocked = false;       // 가드로 막았는지
    bool bWasParried = false;       // 패리했는지
    float ActualDamage = 0.f;       // 실제 적용된 데미지
    EHitReaction AppliedReaction = EHitReaction::None;
    FVector HitPosition;            // 충돌 위치 (월드)
    FVector HitNormal;              // 충돌 법선
};

// ============================================================================
// 전투 이벤트 타입 (VFX/카메라 효과 트리거용)
// ============================================================================
enum class ECombatEventType
{
    // 일반 히트
    NormalHit,              // 일반 공격 적중
    NormalHitReceived,      // 일반 공격 피격

    // 방어
    Blocked,                // 가드 성공 (공격자 시점)
    BlockedHit,             // 가드로 막음 (방어자 시점)
    Parried,                // 패리 성공 (방어자 시점)
    GotParried,             // 패리당함 (공격자 시점)

    // 스킬 관련
    SkillHit,               // 스킬 공격 적중
    SkillParried,           // 스킬 공격 패리됨

    // 회피
    DodgeSuccess,           // 회피 성공 (공격 범위 내에서 회피)

    // 특수 상황
    LastHit,                // 마지막 일격 (적 처치)
    UltimateHit,            // 궁극기 적중 예정
};

// ============================================================================
// 전투 이벤트 정보 (VFX/카메라 시스템으로 전달)
// ============================================================================
struct FCombatEvent
{
    ECombatEventType EventType = ECombatEventType::NormalHit;

    // 관련 액터
    AActor* Attacker = nullptr;     // 공격자
    AActor* Defender = nullptr;     // 방어자/피격자

    // 위치 정보
    FVector HitPosition;            // 충돌/이벤트 발생 위치
    FVector HitDirection;           // 공격 방향
    FVector HitNormal;              // 충돌 법선

    // 추가 정보
    float Damage = 0.f;             // 데미지 (적용 전)
    float ActualDamage = 0.f;       // 실제 적용된 데미지
    EDamageType DamageType = EDamageType::Light;
    bool bIsSkillAttack = false;    // 스킬 공격 여부
    bool bIsFatalBlow = false;      // 마지막 일격 여부 (적 처치)

    FCombatEvent() = default;

    FCombatEvent(ECombatEventType InType, AActor* InAttacker, AActor* InDefender)
        : EventType(InType)
        , Attacker(InAttacker)
        , Defender(InDefender)
    {}
};
