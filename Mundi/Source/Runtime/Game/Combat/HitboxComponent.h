#pragma once

#include "BoxComponent.h"
#include "CombatTypes.h"
#include "IDamageable.h"
#include "Delegates.h"
#include  "UHitboxComponent.generated.h"

// ============================================================================
// UHitboxComponent - 공격 판정용 히트박스 컴포넌트
// ============================================================================
// 사용법:
//   1. 무기나 캐릭터에 붙이기
//   2. 애님 노티파이에서 EnableHitbox() / DisableHitbox() 호출
//   3. 충돌 시 자동으로 TakeDamage 호출
// ============================================================================
UCLASS(DisplayName = "UHitboxComponent", Description = "캐릭터간 히트 컴포넌트 ")
class UHitboxComponent : public UBoxComponent
{
public:
    GENERATED_REFLECTION_BODY()

    UHitboxComponent();
    virtual ~UHitboxComponent() = default;

    // ========================================================================
    // Lifecycle
    // ========================================================================
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime) override;

    // ========================================================================
    // 디버그 렌더링
    // ========================================================================
    virtual void RenderDebugVolume(class URenderer* Renderer) const override;

    // ========================================================================
    // 히트박스 활성화/비활성화
    // ========================================================================

    /**
     * 히트박스를 활성화합니다.
     * @param InDamageInfo 이 공격에 적용할 데미지 정보
     * @param InDuration 지속 시간 (0 이하면 수동 비활성화 필요)
     */
    void EnableHitbox(const FDamageInfo& InDamageInfo, float InDuration = 0.0f);

    /**
     * 히트박스를 비활성화합니다.
     */
    void DisableHitbox();

    /**
     * 히트박스 활성 상태 확인
     */
    bool IsHitboxActive() const { return bIsActive; }

    // ========================================================================
    // 히트 액터 관리 (중복 히트 방지)
    // ========================================================================

    /**
     * 이번 공격에서 이미 맞은 액터 목록을 초기화합니다.
     * (새 공격 시작 시 호출)
     */
    void ClearHitActors();

    /**
     * 해당 액터가 이번 공격에서 이미 맞았는지 확인합니다.
     */
    bool HasAlreadyHit(AActor* Actor) const;

    /**
     * 히트한 액터를 목록에 추가합니다.
     */
    void AddHitActor(AActor* Actor);

    // ========================================================================
    // 설정
    // ========================================================================

    /** 히트박스 소유자 (자신은 맞지 않도록) */
    void SetOwnerActor(AActor* InOwner) { OwnerActor = InOwner; }

  

    /** 히트 시 호출되는 콜백 설정 (기존 방식) */
    using FOnHitCallback = std::function<void(AActor* /*HitActor*/, const FCombatHitResult& /*Result*/)>;
    void SetOnHitCallback(FOnHitCallback Callback) { OnHitCallback = Callback; }

    // ========================================================================
    // 전투 이벤트 델리게이트
    // ========================================================================
    /** 전투 이벤트 발생 시 호출 (VFX, 카메라 효과 등에서 구독) */
    DECLARE_DELEGATE(OnCombatEvent, const FCombatEvent&);

    // ========================================================================
    // 현재 데미지 정보 접근
    // ========================================================================
    const FDamageInfo& GetCurrentDamageInfo() const { return CurrentDamageInfo; }
    void SetDamageInfo(const FDamageInfo& InDamageInfo) { CurrentDamageInfo = InDamageInfo; }

protected:
    /**
     * 충돌 발생 시 호출됩니다.
     * @param OtherActor 충돌한 액터
     * @param HitPosition 충돌 위치 (월드 좌표)
     * @param HitNormal 충돌 법선
     */
    void OnOverlapDetected(AActor* OtherActor, const FVector& HitPosition, const FVector& HitNormal);

    /**
     * 실제 데미지 처리를 수행합니다.
     * @param HitPosition 충돌 위치 (월드 좌표)
     * @param HitNormal 충돌 법선
     */
    FCombatHitResult ProcessHit(AActor* TargetActor, IDamageable* Target, const FVector& HitPosition, const FVector& HitNormal);

    /**
     * PhysX SweepBox로 충돌 감지를 수행합니다.
     */
    void PerformSweepCheck();

private:
    // 현재 공격 정보
    FDamageInfo CurrentDamageInfo;

    // 이번 공격에서 이미 맞은 액터들
    TArray<AActor*> HitActors;

    // 상태
    UPROPERTY(EditAnywhere, Category = "Hitbox")
    bool bIsActive = false;


    // 히트박스 소유자 (자신은 제외)
    AActor* OwnerActor = nullptr;

    // 히트 콜백
    FOnHitCallback OnHitCallback;

    // 자동 비활성화 타이머
    float RemainingDuration = 0.0f;
    bool bUseAutoDisable = false;

    // 디버그용 - 히트 정보 저장
    struct FDebugHitInfo
    {
        FVector Position;
        FVector Normal;
        float RemainingTime;
    };
    TArray<FDebugHitInfo> DebugHitInfos;
};
