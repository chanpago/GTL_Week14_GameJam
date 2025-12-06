#pragma once
#include "Object.h"
#include "AnimTypes.h"

// Forward declarations
class UAnimationStateMachine;
class UAnimSequence;
class UAnimMontage;
class USkeletalMeshComponent;

/**
 * @brief 애니메이션 재생 상태를 관리하는 구조체
 * @note 포즈 제공자 기반으로 리팩터링됨 - AnimSequence, BlendSpace 등 모두 지원
 */
struct FAnimationPlayState
{
    /** 포즈를 제공하는 소스 (AnimSequence, BlendSpace1D 등) */
    IAnimPoseProvider* PoseProvider = nullptr;

    /** 기존 호환성을 위한 AnimSequence 참조 (노티파이 등에서 사용) */
    UAnimSequence* Sequence = nullptr;

    /** BlendSpace용 파라미터 값 */
    float BlendParameter = 0.0f;

    float CurrentTime = 0.0f;
    float PlayRate = 1.0f;
    float BlendWeight = 1.0f;
    bool bIsLooping = false;
    bool bIsPlaying = false;
};

/**
 * @brief 몽타주 재생 상태를 관리하는 구조체
 * @note 상태머신과 별개로 동작하며, 상태머신 위에 오버레이됨
 */
struct FMontagePlayState
{
    /** 재생할 몽타주 */
    UAnimMontage* Montage = nullptr;

    /** 현재 재생 시간 */
    float CurrentTime = 0.0f;

    /** 이전 프레임 재생 시간 (노티파이 검출용) */
    float PreviousTime = 0.0f;

    /** 재생 속도 */
    float PlayRate = 1.0f;

    /** 재생 중인지 여부 */
    bool bIsPlaying = false;

    /** 블렌드 인 시간 */
    float BlendInTime = 0.1f;

    /** 블렌드 아웃 시간 */
    float BlendOutTime = 0.1f;

    /** 현재 블렌드 가중치 (0~1) */
    float CurrentWeight = 0.0f;

    /** 블렌드 아웃 시작 시간 (PlayLength - BlendOutTime) */
    float BlendOutStartTime = 0.0f;

    /** 실제 재생 길이 (끝 프레임 자르기 적용됨) */
    float EffectivePlayLength = 0.0f;

    /** 강제 블렌드 아웃 중인지 (Montage_Stop 호출됨) */
    bool bIsBlendingOut = false;
};

/**
 * @brief 애니메이션 인스턴스
 * - 스켈레탈 메시 컴포넌트의 애니메이션 재생을 관리
 * - 시퀀스 재생, 블렌딩, 노티파이 처리 등을 담당
 *
 * @example 사용법 1: UE 스타일 단일 애니메이션 재생 (가장 간단)
 *
 * // SkeletalMeshComponent에서 직접 PlayAnimation 호출
 * // 내부적으로 AnimationSingleNode 모드로 전환하고 UAnimSingleNodeInstance 생성
 * UAnimSequence* WalkAnim = RESOURCE.Get<UAnimSequence>("Data/Character_Walk.fbx_Walk");
 * SkeletalMeshComponent->PlayAnimation(WalkAnim, true);
 *
 *
 * @example 사용법 2: AnimSingleNodeInstance를 직접 사용
 *
 * // 1. AnimSingleNodeInstance 생성 및 초기화
 * UAnimSingleNodeInstance* AnimInstance = NewObject<UAnimSingleNodeInstance>();
 * SkeletalMeshComponent->SetAnimInstance(AnimInstance);
 *
 * // 2. 애니메이션 재생
 * UAnimSequence* IdleAnim = RESOURCE.Get<UAnimSequence>("Data/Character_Idle.fbx_Idle");
 * AnimInstance->PlaySingleNode(IdleAnim, true, 1.0f);
 *
 * // 3. 다른 애니메이션으로 전환
 * UAnimSequence* RunAnim = RESOURCE.Get<UAnimSequence>("Data/Character_Run.fbx_Run");
 * AnimInstance->PlaySingleNode(RunAnim, true, 1.5f); // 1.5배속
 *
 *
 * @example 사용법 3: 상태머신 사용 (복잡한 애니메이션 그래프)
 *
 * // 1. 커스텀 AnimInstance 생성 (또는 UAnimInstance 직접 사용)
 * UAnimInstance* AnimInstance = NewObject<UAnimInstance>();
 * SkeletalMeshComponent->SetAnimInstance(AnimInstance);
 *
 * // 2. 애니메이션 시퀀스 로드
 * UAnimSequence* IdleAnim = RESOURCE.Get<UAnimSequence>("Data/Character_Idle.fbx_Idle");
 * UAnimSequence* WalkAnim = RESOURCE.Get<UAnimSequence>("Data/Character_Walk.fbx_Walk");
 * UAnimSequence* RunAnim = RESOURCE.Get<UAnimSequence>("Data/Character_Run.fbx_Run");
 *
 * // 3. 상태머신 생성 및 설정
 * UAnimationStateMachine* StateMachine = NewObject<UAnimationStateMachine>();
 * StateMachine->Initialize(AnimInstance);
 *
 * // 4. 상태 추가
 * StateMachine->AddState(FAnimationState("Idle", IdleAnim, true, 1.0f));
 * StateMachine->AddState(FAnimationState("Walk", WalkAnim, true, 1.0f));
 * StateMachine->AddState(FAnimationState("Run", RunAnim, true, 1.0f));
 *
 * // 5. 전이 조건 추가
 * StateMachine->AddTransition(FStateTransition("Idle", "Walk",
 *     [AnimInstance]() {
 *         return AnimInstance->GetMovementSpeed() > 0.0f && AnimInstance->GetMovementSpeed() < 5.0f;
 *     }, 0.2f));
 *
 * StateMachine->AddTransition(FStateTransition("Walk", "Run",
 *     [AnimInstance]() { return AnimInstance->GetMovementSpeed() >= 5.0f; }, 0.3f));
 *
 * StateMachine->AddTransition(FStateTransition("Run", "Walk",
 *     [AnimInstance]() { return AnimInstance->GetMovementSpeed() < 5.0f; }, 0.2f));
 *
 * StateMachine->AddTransition(FStateTransition("Walk", "Idle",
 *     [AnimInstance]() { return AnimInstance->GetMovementSpeed() <= 0.0f; }, 0.2f));
 *
 * // 6. 초기 상태 설정 및 상태머신 연결
 * StateMachine->SetInitialState("Idle");
 * AnimInstance->SetStateMachine(StateMachine);
 *
 * // 7. 게임플레이 루프에서 파라미터 업데이트
 * // (TickComponent는 자동으로 AnimInstance->NativeUpdateAnimation 호출)
 * AnimInstance->SetMovementSpeed(PlayerVelocity.Length());
 * AnimInstance->SetIsMoving(PlayerVelocity.Length() > 0.1f);
 */
class UAnimInstance : public UObject
{
    DECLARE_CLASS(UAnimInstance, UObject)

    // 루트 모션 관련 멤버에 접근할 수 있도록 허용
    friend class USkeletalMeshComponent;
    friend class UAnimationStateMachine;

public:
    UAnimInstance() = default;
    virtual ~UAnimInstance() = default;

    // ============================================================
    // Initialization & Setup
    // ============================================================

    /**
     * @brief AnimInstance 초기화
     * @param InComponent 소유 컴포넌트
     */
    virtual void Initialize(USkeletalMeshComponent* InComponent);

    // ============================================================
    // Update & Pose Evaluation
    // ============================================================

    /**
     * @brief 애니메이션 업데이트 (매 프레임 호출)
     * @param DeltaSeconds 프레임 시간
     */
    virtual void NativeUpdateAnimation(float DeltaSeconds);

    /**
     * @brief 현재 포즈를 평가하여 반환
     * @param OutPose 출력 포즈
     */
    void EvaluatePose(TArray<FTransform>& OutPose);

    // ============================================================
    // Playback API
    // ============================================================

    /**
     * @brief 애니메이션 시퀀스 재생
     * @param Sequence 재생할 시퀀스
     * @param bLoop 루프 여부
     * @param InPlayRate 재생 속도
     */
    void PlaySequence(UAnimSequence* Sequence, bool bLoop = true, float InPlayRate = 1.0f);

    /**
    * @brief 애니메이션 시퀀스 재생 + Bone 기준으로 서로 다른 애니메이션을 위한 함수
    */
    void PlaySequence(UAnimSequence* Sequence, EAnimLayer Layer, bool bLoop = true, float InPlayRate = 1.0f);
    /**
     * @brief 현재 재생 중인 시퀀스 정지
     */
    void StopSequence();

    /**
     * @brief 다른 시퀀스로 블렌드
     * @param Sequence 블렌드할 시퀀스
     * @param bLoop 타겟 시퀀스 루프 여부
     * @param InPlayRate 타겟 시퀀스 재생 속도
     * @param BlendTime 블렌드 시간
     */
    void BlendTo(UAnimSequence* Sequence, bool bLoop, float InPlayRate, float BlendTime);

    /**
    * @brief 다른 시퀀스로 블렌드 + 다른 애니메이션 사용하기 위한 함수
    */
    void BlendTo(UAnimSequence* Sequence, EAnimLayer Layer, bool bLoop, float InPlayRate, float BlendTime);

    /**
     * @brief PoseProvider 재생 (BlendSpace 등)
     * @param Provider 포즈 제공자
     * @param bLoop 루프 여부
     * @param InPlayRate 재생 속도
     */
    void PlayPoseProvider(IAnimPoseProvider* Provider, bool bLoop = true, float InPlayRate = 1.0f);

    /**
     * @brief PoseProvider로 블렌드 (BlendSpace 등)
     * @param Provider 포즈 제공자
     * @param bLoop 루프 여부
     * @param InPlayRate 재생 속도
     * @param BlendTime 블렌드 시간
     */
    void BlendToPoseProvider(IAnimPoseProvider* Provider, bool bLoop, float InPlayRate, float BlendTime);

    /**
     * @brief 재생 중인지 확인
     */
    bool IsPlaying() const { return CurrentPlayState.bIsPlaying; }

    /**
     * @brief 현재 재생 중인 시퀀스 반환
     */
    UAnimSequence* GetCurrentSequence() const { return CurrentPlayState.Sequence; }

    // 상/하체 분리 설정
    void EnableUpperBodySplit(FName BoneName);

    // ============================================================
    // Notify & Curve Processing
    // ============================================================

    /**
     * @brief 애니메이션 노티파이 트리거
     * @param DeltaSeconds 프레임 시간
     */
    void TriggerAnimNotifies(float DeltaSeconds);

    /**
     * @brief 애니메이션 커브 값 업데이트
     */
    void UpdateAnimationCurves();

    // ============================================================
    // State Machine & Parameters
    // ============================================================

    /**
     * @brief 상태머신 가져오기
     */
    UAnimationStateMachine* GetStateMachine() const { return AnimStateMachine; }

    /**
     * @brief 상태머신 설정
     */
    void SetStateMachine(UAnimationStateMachine* InStateMachine);

    /**
     * @brief 이동 속도 설정 (상태머신 전이 조건용)
     */
    void SetMovementSpeed(float Speed) { MovementSpeed = Speed; }

    /**
     * @brief 이동 속도 가져오기
     */
    float GetMovementSpeed() const { return MovementSpeed; }

    /**
     * @brief 이동 모드 설정 (상태머신 전이 조건용)
     */
    void SetIsMoving(bool bInMoving) { bIsMoving = bInMoving; }

    /**
     * @brief 이동 중인지 확인
     */
    bool GetIsMoving() const { return bIsMoving; }

    // ============================================================
    // Root Motion
    // ============================================================

    /**
     * @brief 루트 모션 활성화/비활성화
     * @param bEnabled true면 루트 모션 활성화
     */
    void SetRootMotionEnabled(bool bEnabled) { bEnableRootMotion = bEnabled; }

    /**
     * @brief 루트 모션이 활성화되어 있는지 확인
     */
    bool IsRootMotionEnabled() const { return bEnableRootMotion; }

    /**
     * @brief 애니메이션 끝에서 자를 시간 설정 (초 단위)
     */
    void SetAnimationCutEndTime(float InSeconds) { AnimationCutEndTime = InSeconds; }

    /**
     * @brief 애니메이션 끝에서 자를 시간 반환 (초 단위)
     */
    float GetAnimationCutEndTime() const { return AnimationCutEndTime; }

    /**
     * @brief 루트 모션 이동 델타를 가져오고 리셋
     * @return 이번 프레임의 루트 본 이동량 (월드 스페이스)
     */
    FVector ConsumeRootMotionTranslation();

    /**
     * @brief 루트 모션 회전 델타를 가져오고 리셋
     * @return 이번 프레임의 루트 본 회전량
     */
    FQuat ConsumeRootMotionRotation();

    /**
     * @brief 현재 루트 모션 이동 델타 (리셋 없이)
     */
    FVector GetRootMotionTranslation() const { return RootMotionTranslation; }

    /**
     * @brief 현재 루트 모션 회전 델타 (리셋 없이)
     */
    FQuat GetRootMotionRotation() const { return RootMotionRotation; }

    // ============================================================
    // Montage API
    // ============================================================

    /**
     * @brief 몽타주 재생
     * @param Montage 재생할 몽타주
     * @param BlendIn 블렌드 인 시간 (기본 0.1초)
     * @param BlendOut 블렌드 아웃 시간 (기본 0.1초)
     * @param PlayRate 재생 속도 (기본 1.0)
     *
     * @example
     * // 피격 시
     * AnimInstance->Montage_Play(HitMontage, 0.1f, 0.2f);
     *
     * // 공격 시
     * AnimInstance->Montage_Play(AttackMontage, 0.05f, 0.1f, 1.2f);
     */
    void Montage_Play(UAnimMontage* Montage, float BlendIn = 0.1f, float BlendOut = 0.1f, float PlayRate = 1.0f);

    /**
     * @brief 현재 재생 중인 몽타주 정지
     * @param BlendOut 블렌드 아웃 시간 (기본 0.1초)
     */
    void Montage_Stop(float BlendOut = 0.1f);

    /**
     * @brief 몽타주가 재생 중인지 확인
     * @return true면 몽타주 재생 중
     */
    bool Montage_IsPlaying() const;

    /**
     * @brief 현재 몽타주 재생 위치 반환
     * @return 현재 재생 시간 (초)
     */
    float Montage_GetPosition() const;

    /**
     * @brief 현재 재생 중인 몽타주 반환
     * @return 몽타주 (없으면 nullptr)
     */
    UAnimMontage* Montage_GetCurrentMontage() const;

    /**
     * @brief 몽타주가 블렌드 아웃 중인지 확인
     * @return true면 블렌드 아웃 중
     */
    bool Montage_IsBlendingOut() const;

    /**
     * @brief 현재 재생 중인 몽타주의 재생 속도를 변경
     * @param NewPlayRate 새로운 재생 속도 (1.0 = 정상 속도)
     */
    void Montage_SetPlayRate(float NewPlayRate);

    // ============================================================
    // Getters
    // ============================================================

    USkeletalMeshComponent* GetOwningComponent() const { return OwningComponent; }

protected:
    // PlayState 헬퍼
    void EvaluatePoseForState(const FAnimationPlayState& PlayState, TArray<FTransform>& OutPose, float DeltaTime = 0.0f) const;
    void AdvancePlayState(FAnimationPlayState& PlayState, float DeltaSeconds);
    void BlendPoseArrays(const TArray<FTransform>& FromPose, const TArray<FTransform>& ToPose, float Alpha, TArray<FTransform>& OutPose) const;
    void GetPoseForLayer(int32 LayerIndex, TArray<FTransform>& OutPose, float DeltaSeconds);

    // 몽타주 헬퍼
    TArray<FTransform> ProcessMontage(const TArray<FTransform>& BasePose, float DeltaSeconds);

    // 소유 컴포넌트
    USkeletalMeshComponent* OwningComponent = nullptr;

    // 현재 재생 상태
    FAnimationPlayState CurrentPlayState;

    // 블렌딩 상태 (향후 확장용)
    FAnimationPlayState BlendTargetState;
    float BlendTimeRemaining = 0.0f;
    float BlendTotalTime = 0.0f;

    // 이전 프레임 재생 시간 (노티파이 검출용)
    float PreviousPlayTime = 0.0f;

    // 스켈레톤 참조
    FSkeleton* CurrentSkeleton = nullptr;

    // 상태머신
    UAnimationStateMachine* AnimStateMachine = nullptr;

    // 애니메이션 파라미터 (상태머신 전이 조건용)
    float MovementSpeed = 0.0f;
    bool bIsMoving = false;

    // 루트 모션 관련
    bool bEnableRootMotion = false;
    FVector RootMotionTranslation;
    FQuat RootMotionRotation = FQuat::Identity();
    FTransform PreviousRootBoneTransform;
    bool bHasPreviousRootTransform = false;
    float AnimationCutEndTime = 0.0f;  // 애니메이션 끝에서 자를 시간 (초 단위)

    // 레이어별 상태 관리
    // Layer[0] = Base, Layer[1] = Upper
    FAnimationPlayState Layers[(int32)EAnimLayer::Count];
    FAnimationPlayState BlendTargets[(int32)EAnimLayer::Count];

    // 각 레이어 별 블렌딩 목표 상태
    float LayerBlendTimeRemaining[(int32)EAnimLayer::Count] = { 0.0f };
    float LayerBlendTotalTime[(int32)EAnimLayer::Count] = { 0.0f };

    //마스킹 데이터
    bool bUseUpperBody = false;
    TArray<bool> UpperBodyMask; // true면 상체

    // ============================================================
    // Montage
    // ============================================================

    /** 몽타주 재생 상태 */
    FMontagePlayState MontageState;

    /** 몽타주가 활성화되어 있는지 여부 */
    bool bMontageActive = false;
};

