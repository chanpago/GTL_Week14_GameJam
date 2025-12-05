# Day 1 - 전투 시스템 통일 규격 설계서

## 목차
1. [폴더 구조](#1-폴더-구조)
2. [공격 판정 인터페이스](#2-공격-판정-인터페이스)
3. [스탯 컴포넌트](#3-스탯-컴포넌트)
4. [애님 노티파이 규칙](#4-애님-노티파이-규칙)
5. [팀원별 작업 범위](#5-팀원별-작업-범위)

---

## 1. 폴더 구조

```
Mundi/Source/Runtime/Game/           <-- 새로 생성
├── Combat/
│   ├── CombatTypes.h                # 공용 열거형, 구조체
│   ├── IDamageable.h                # 데미지 인터페이스
│   ├── DamageInfo.h                 # 데미지 정보 구조체
│   ├── HitboxComponent.h/cpp        # 공격 판정 컴포넌트
│   └── StatsComponent.h/cpp         # HP/스태미나 컴포넌트
├── Player/
│   ├── PlayerCharacter.h/cpp        # [팀원 A]
│   └── PlayerAnimInstance.h/cpp
├── Enemy/
│   ├── EnemyBase.h/cpp              # [팀원 B]
│   ├── EnemyAIController.h/cpp
│   └── EnemyAnimInstance.h/cpp
└── UI/
    ├── GameHUD.h/cpp                # [팀원 C]
    └── HealthBarWidget.h/cpp
```

---

## 2. 공격 판정 인터페이스

### 2.1 데미지 정보 구조체 (DamageInfo.h)

```cpp
// 공격 타입
enum class EDamageType
{
    Light,      // 약공격 - 경직 짧음
    Heavy,      // 강공격 - 경직 김
    Special,    // 특수기 - 넉백
    Parried     // 패리당함 - 큰 경직
};

// 피격 반응 타입
enum class EHitReaction
{
    None,       // 반응 없음 (슈퍼아머)
    Flinch,     // 살짝 경직
    Stagger,    // 큰 경직
    Knockback,  // 넉백
    Knockdown   // 넘어짐
};

// 데미지 정보 (공격자 → 피격자로 전달)
struct FDamageInfo
{
    AActor* Instigator;         // 공격한 액터
    AActor* DamageCauser;       // 데미지 원인 (무기 등)

    float Damage;               // 데미지 양
    EDamageType DamageType;     // 공격 타입
    EHitReaction HitReaction;   // 요청하는 피격 반응

    FVector HitLocation;        // 맞은 위치
    FVector HitDirection;       // 맞은 방향 (넉백용)

    float StaggerDuration;      // 경직 시간
    float KnockbackForce;       // 넉백 힘

    bool bCanBeBlocked;         // 가드 가능 여부
    bool bCanBeParried;         // 패리 가능 여부
};
```

### 2.2 데미지 인터페이스 (IDamageable.h)

```cpp
// 모든 데미지 받는 액터가 구현해야 하는 인터페이스
class IDamageable
{
public:
    // 데미지 받기 - 플레이어, 적 모두 이걸 구현
    virtual float TakeDamage(const FDamageInfo& DamageInfo) = 0;

    // 현재 상태 조회
    virtual bool IsAlive() const = 0;
    virtual bool CanBeHit() const = 0;      // 무적 상태 체크
    virtual bool IsBlocking() const = 0;    // 가드 중인지
    virtual bool IsParrying() const = 0;    // 패리 타이밍인지

    // 피격 반응
    virtual void OnHitReaction(EHitReaction Reaction, const FDamageInfo& DamageInfo) = 0;
};
```

### 2.3 히트박스 컴포넌트 (HitboxComponent.h)

```cpp
// 무기/공격에 붙이는 컴포넌트
class UHitboxComponent : public UBoxComponent
{
public:
    // 공격 시작/종료 (애님 노티파이에서 호출)
    void EnableHitbox(const FDamageInfo& InDamageInfo);
    void DisableHitbox();

    // 이미 맞은 액터 관리 (중복 히트 방지)
    void ClearHitActors();
    bool HasAlreadyHit(AActor* Actor);

    // 충돌 시 자동 호출
    void OnOverlapBegin(/*...*/)
    {
        if (IDamageable* Target = Cast<IDamageable>(OtherActor))
        {
            if (!HasAlreadyHit(OtherActor))
            {
                Target->TakeDamage(CurrentDamageInfo);
                AddHitActor(OtherActor);
            }
        }
    }

private:
    FDamageInfo CurrentDamageInfo;
    TArray<AActor*> HitActors;  // 이번 공격에서 이미 맞은 액터들
    bool bIsActive = false;
};
```

---

## 3. 스탯 컴포넌트

### 3.1 스탯 컴포넌트 (StatsComponent.h)

```cpp
// 델리게이트 선언 (UI 업데이트용)
DECLARE_DELEGATE(FOnHealthChanged, float /*Current*/, float /*Max*/);
DECLARE_DELEGATE(FOnStaminaChanged, float /*Current*/, float /*Max*/);
DECLARE_DELEGATE(FOnDeath);

class UStatsComponent : public UActorComponent
{
public:
    // ========== HP 관련 ==========
    float MaxHealth = 100.f;
    float CurrentHealth = 100.f;

    void ApplyDamage(float Damage);
    void Heal(float Amount);
    bool IsAlive() const { return CurrentHealth > 0.f; }
    float GetHealthPercent() const { return CurrentHealth / MaxHealth; }

    // ========== 스태미나 관련 ==========
    float MaxStamina = 100.f;
    float CurrentStamina = 100.f;
    float StaminaRegenRate = 20.f;      // 초당 회복량
    float StaminaRegenDelay = 1.0f;     // 사용 후 회복 시작까지 딜레이

    bool ConsumeStamina(float Amount);  // 사용 가능하면 true
    bool HasEnoughStamina(float Amount) const;
    float GetStaminaPercent() const { return CurrentStamina / MaxStamina; }

    // ========== 스태미나 비용 (밸런싱용) ==========
    float DodgeCost = 25.f;
    float LightAttackCost = 15.f;
    float HeavyAttackCost = 30.f;
    float BlockCostPerHit = 20.f;

    // ========== 이벤트 ==========
    FOnHealthChanged OnHealthChanged;
    FOnStaminaChanged OnStaminaChanged;
    FOnDeath OnDeath;

    // ========== Tick ==========
    virtual void TickComponent(float DeltaTime) override
    {
        // 스태미나 자동 회복
        if (TimeSinceStaminaUse > StaminaRegenDelay)
        {
            CurrentStamina = FMath::Min(MaxStamina,
                CurrentStamina + StaminaRegenRate * DeltaTime);
            OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
        }
        TimeSinceStaminaUse += DeltaTime;
    }

private:
    float TimeSinceStaminaUse = 0.f;
};
```

### 3.2 사용 예시

```cpp
// PlayerCharacter.cpp
void APlayerCharacter::AttemptDodge()
{
    if (StatsComponent->ConsumeStamina(StatsComponent->DodgeCost))
    {
        // 구르기 실행
        StartDodge();
    }
    else
    {
        // 스태미나 부족 피드백
        PlayStaminaDepletedFeedback();
    }
}

// EnemyBase.cpp - TakeDamage 구현
float AEnemyBase::TakeDamage(const FDamageInfo& DamageInfo)
{
    StatsComponent->ApplyDamage(DamageInfo.Damage);

    if (!StatsComponent->IsAlive())
    {
        Die();
        return DamageInfo.Damage;
    }

    OnHitReaction(DamageInfo.HitReaction, DamageInfo);
    return DamageInfo.Damage;
}
```

---

## 4. 애님 노티파이 규칙

### 4.1 노티파이 네이밍 규칙

| 노티파이 이름 | 타입 | 용도 | 호출 시점 |
|--------------|------|------|----------|
| `AN_EnableHitbox` | Notify | 히트박스 활성화 | 공격 모션 시작점 |
| `AN_DisableHitbox` | Notify | 히트박스 비활성화 | 공격 모션 끝점 |
| `AN_ComboWindow_Start` | Notify | 콤보 입력 윈도우 시작 | 공격 중반 |
| `AN_ComboWindow_End` | Notify | 콤보 입력 윈도우 종료 | 공격 후반 |
| `AN_AttackEnd` | Notify | 공격 종료 (Idle 복귀 가능) | 애니메이션 끝 |
| `AN_FootStep` | Notify | 발소리 재생 | 발 닿는 시점 |
| `AN_PlaySound` | Notify | 사운드 재생 | 필요 시점 |
| `ANS_Invincible` | NotifyState | 무적 시간 (구르기) | 구르기 시작~끝 |
| `ANS_SuperArmor` | NotifyState | 슈퍼아머 (경직 무시) | 강공격 중 |
| `ANS_CanParry` | NotifyState | 패리 가능 시간 | 가드 시작 직후 |

### 4.2 노티파이 클래스 구현

```cpp
// ===== AN_EnableHitbox.h =====
class UAN_EnableHitbox : public UAnimNotify
{
public:
    // 에디터에서 설정
    EDamageType DamageType = EDamageType::Light;
    float Damage = 10.f;
    float StaggerDuration = 0.3f;
    EHitReaction HitReaction = EHitReaction::Flinch;

    virtual void Notify(USkeletalMeshComponent* MeshComp,
                        UAnimSequenceBase* Animation) override
    {
        if (AActor* Owner = MeshComp->GetOwner())
        {
            if (UHitboxComponent* Hitbox = Owner->GetComponent<UHitboxComponent>())
            {
                FDamageInfo Info;
                Info.Instigator = Owner;
                Info.Damage = Damage;
                Info.DamageType = DamageType;
                Info.HitReaction = HitReaction;
                Info.StaggerDuration = StaggerDuration;

                Hitbox->EnableHitbox(Info);
            }
        }
    }
};

// ===== AN_DisableHitbox.h =====
class UAN_DisableHitbox : public UAnimNotify
{
    virtual void Notify(USkeletalMeshComponent* MeshComp,
                        UAnimSequenceBase* Animation) override
    {
        if (AActor* Owner = MeshComp->GetOwner())
        {
            if (UHitboxComponent* Hitbox = Owner->GetComponent<UHitboxComponent>())
            {
                Hitbox->DisableHitbox();
                Hitbox->ClearHitActors();
            }
        }
    }
};

// ===== ANS_Invincible.h (State) =====
class UANS_Invincible : public UAnimNotifyState
{
    virtual void NotifyBegin(...) override
    {
        if (auto* Combat = Owner->GetComponent<UCombatComponent>())
            Combat->SetInvincible(true);
    }

    virtual void NotifyEnd(...) override
    {
        if (auto* Combat = Owner->GetComponent<UCombatComponent>())
            Combat->SetInvincible(false);
    }
};
```

### 4.3 애니메이션 노티파이 배치 가이드

```
약공격 (0.5초 애니메이션 기준)
├─ 0.0s: 애니메이션 시작
├─ 0.15s: AN_EnableHitbox (DamageType::Light, Damage=10)
├─ 0.25s: AN_DisableHitbox
├─ 0.3s: AN_ComboWindow_Start
├─ 0.45s: AN_ComboWindow_End
└─ 0.5s: AN_AttackEnd

강공격 (1.0초 애니메이션 기준)
├─ 0.0s: 애니메이션 시작
├─ 0.0s~0.4s: ANS_SuperArmor (경직 무시)
├─ 0.35s: AN_EnableHitbox (DamageType::Heavy, Damage=30)
├─ 0.55s: AN_DisableHitbox
├─ 0.6s: AN_ComboWindow_Start
├─ 0.85s: AN_ComboWindow_End
└─ 1.0s: AN_AttackEnd

구르기 (0.6초 애니메이션 기준)
├─ 0.0s: 애니메이션 시작
├─ 0.05s~0.4s: ANS_Invincible (무적)
└─ 0.6s: AN_DodgeEnd
```

---

## 5. 팀원별 작업 범위

### 5.1 공용 (Day 1 오전에 같이 만들기)

| 파일 | 우선순위 | 담당 |
|------|---------|------|
| `CombatTypes.h` | 필수 | 전원 합의 |
| `DamageInfo.h` | 필수 | 전원 합의 |
| `IDamageable.h` | 필수 | 전원 합의 |
| `StatsComponent.h/cpp` | 필수 | 전원 합의 |
| `HitboxComponent.h/cpp` | 필수 | 전원 합의 |

### 5.2 팀원 A - 플레이어

```
담당 파일:
├── PlayerCharacter.h/cpp
├── PlayerAnimInstance.h/cpp
└── 플레이어 관련 AnimNotify

구현할 것:
├── IDamageable 인터페이스 구현
├── 이동 (WASD + 마우스)
├── 약공격 (마우스 좌클릭)
├── 강공격 (마우스 좌클릭 홀드)
├── 구르기 (Space)
├── 가드 (마우스 우클릭)
└── 콤보 시스템 (상태머신)

의존성:
├── StatsComponent (공용)
├── HitboxComponent (공용)
└── InputManager (기존)
```

### 5.3 팀원 B - 적 AI

```
담당 파일:
├── EnemyBase.h/cpp
├── EnemyAIController.h/cpp
├── EnemyAnimInstance.h/cpp
└── BossEnemy.h/cpp (Day 4)

구현할 것:
├── IDamageable 인터페이스 구현
├── AI 상태머신 (Idle → Chase → Attack → Stagger → Dead)
├── 플레이어 감지 (거리 기반)
├── 공격 패턴 2~3개
├── 피격 반응 (Flinch, Stagger)
└── 사망 처리 (Ragdoll 활용)

의존성:
├── StatsComponent (공용)
├── HitboxComponent (공용)
└── AnimationStateMachine (기존)
```

### 5.4 팀원 C - UI & 락온

```
담당 파일:
├── GameHUD.h/cpp
├── HealthBarWidget.h/cpp
├── StaminaBarWidget.h/cpp
├── BossHealthBar.h/cpp
└── LockOnComponent.h/cpp

구현할 것:
├── 플레이어 HP 바
├── 플레이어 스태미나 바
├── 보스 HP 바 (화면 상단)
├── 락온 시스템 (Tab 토글)
├── 락온 타겟 전환 (Q/E)
└── 락온 시 카메라 추종

의존성:
├── StatsComponent.OnHealthChanged (공용)
├── StatsComponent.OnStaminaChanged (공용)
└── Slate 위젯 시스템 (기존)
```

### 5.5 팀원 D - 레벨 & 폴리시

```
담당 파일:
├── 테스트 레벨 프리팹
├── CameraShakeComponent.h/cpp (또는 기존 활용)
└── HitEffectManager.h/cpp

구현할 것:
├── 테스트용 아레나 레벨
├── 적 스폰 포인트 배치
├── 히트 이펙트 (데칼 또는 파티클)
├── 카메라 쉐이크
├── 히트스톱 연출 (기존 RequestHitStop() 활용)
└── 사운드 트리거

의존성:
├── PlayerCameraManager (기존)
├── 데칼 시스템 (기존)
└── 파티클 시스템 (기존)
```

---

## 6. 회의 체크리스트

### Day 1 오전 회의에서 결정할 것

- [ ] `EDamageType` 종류 확정 (Light, Heavy, Special, Parried?)
- [ ] `EHitReaction` 종류 확정 (Flinch, Stagger, Knockback, Knockdown?)
- [ ] 스태미나 비용 수치 (구르기 25, 약공 15, 강공 30?)
- [ ] 기본 HP 수치 (플레이어 100, 일반적 50, 보스 500?)
- [ ] 노티파이 네이밍 규칙 최종 확정
- [ ] 공용 파일 누가 먼저 만들지 역할 분담

### Day 1 오후까지 완료 목표

- [ ] 공용 파일 5개 완성 (Combat 폴더)
- [ ] [팀원 A] 플레이어 이동 + 기본 공격 1개
- [ ] [팀원 B] 적 스폰 + Idle 상태
- [ ] [팀원 C] HP 바 화면에 표시
- [ ] [팀원 D] 테스트 레벨 기본 구성

---

## 7. 기존 엔진에서 활용할 것

| 기능 | 파일 위치 | 사용법 |
|------|----------|--------|
| 캐릭터 기본 | `Core/Object/Character.h` | 상속해서 사용 |
| 입력 시스템 | `InputCore/InputManager.h` | `INPUT.IsKeyPressed()` |
| 애님 상태머신 | `Animation/AnimationStateMachine.h` | 그대로 활용 |
| 히트스톱 | `GameFramework/World.h` | `World->RequestHitStop()` |
| 충돌 델리게이트 | `Core/Object/Actor.h` | `OnComponentBeginOverlap` |
| 박스 충돌 | `Components/BoxComponent.h` | 히트박스로 활용 |
| 애님 노티파이 | `Animation/AnimNotify.h` | 상속해서 확장 |

---

## 8. 데이터 흐름도

```
[플레이어 공격]
    │
    ▼
AN_EnableHitbox (노티파이)
    │
    ▼
HitboxComponent.EnableHitbox()
    │
    ▼
OnComponentBeginOverlap (충돌 감지)
    │
    ▼
IDamageable.TakeDamage(FDamageInfo)
    │
    ├──▶ StatsComponent.ApplyDamage()
    │         │
    │         ▼
    │    OnHealthChanged.Broadcast() ──▶ [UI 업데이트]
    │
    └──▶ OnHitReaction()
              │
              ▼
         [피격 애니메이션 재생]
         [히트스톱/이펙트]
```

---

## 9. 참고: 스텔라 블레이드 핵심 메카닉

1. **패리 시스템**: 적 공격 직전 가드 → 큰 경직 유발
2. **회피**: i-frame 있는 구르기
3. **콤보**: 약공 연타 + 강공 피니시
4. **스태미나**: 공격/회피에 소모, 자동 회복
5. **락온**: 보스전 필수, 카메라 자동 추종

---

*작성일: 2024년 / Mundi 엔진 기반*
