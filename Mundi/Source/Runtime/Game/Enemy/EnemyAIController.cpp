#include "pch.h"
#include "EnemyAIController.h"
#include "EnemyBase.h"
#include "World.h"
#include "PlayerController.h"

AEnemyAIController::AEnemyAIController()
{
}

void AEnemyAIController::BeginPlay()
{
    Super::BeginPlay();
}

void AEnemyAIController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // 쿨타임 업데이트
    if (AttackCooldownTimer > 0.f)
    {
        AttackCooldownTimer -= DeltaSeconds;
    }
}

void AEnemyAIController::Possess(APawn* InPawn)
{
    Super::Possess(InPawn);

    // EnemyBase에서 설정 가져오기
    if (AEnemyBase* Enemy = GetEnemyPawn())
    {
        // 필요시 Enemy의 설정을 컨트롤러로 복사
    }
}

// ============================================================================
// AI 상태 관리
// ============================================================================

void AEnemyAIController::SetBehaviorState(EAIBehaviorState NewState)
{
    if (BehaviorState == NewState)
    {
        return;
    }

    PreviousState = BehaviorState;
    BehaviorState = NewState;

    // 상태 전환 시 처리
    switch (NewState)
    {
    case EAIBehaviorState::Chase:
        // 추적 시작 시 타겟 방향 바라보기
        if (TargetActor)
        {
            FocusOnActor(TargetActor);
        }
        break;

    case EAIBehaviorState::Attack:
        bIsAttacking = true;
        break;

    case EAIBehaviorState::Strafe:
        StrafeTimer = StrafeDuration;
        StrafeDirection = (rand() % 2 == 0) ? 1.f : -1.f;
        break;

    case EAIBehaviorState::Dead:
        StopMovement();
        bAIEnabled = false;
        break;

    default:
        break;
    }

    // 이전 상태 정리
    if (PreviousState == EAIBehaviorState::Attack)
    {
        bIsAttacking = false;
    }
}

// ============================================================================
// 전투
// ============================================================================

void AEnemyAIController::RequestAttack()
{
    if (AttackCooldownTimer <= 0.f && !bIsAttacking)
    {
        bAttackRequested = true;
    }
}

void AEnemyAIController::EnterStagger(float Duration)
{
    SetBehaviorState(EAIBehaviorState::Stagger);
    StaggerTimer = Duration;
    StopMovement();
}

void AEnemyAIController::OnPawnDeath()
{
    SetBehaviorState(EAIBehaviorState::Dead);
}

// ============================================================================
// AI 로직
// ============================================================================

void AEnemyAIController::UpdateAI(float DeltaTime)
{
    if (!bAIEnabled || BehaviorState == EAIBehaviorState::Dead)
    {
        return;
    }

    switch (BehaviorState)
    {
    case EAIBehaviorState::Idle:
        UpdateIdle(DeltaTime);
        break;
    case EAIBehaviorState::Patrol:
        UpdatePatrol(DeltaTime);
        break;
    case EAIBehaviorState::Chase:
        UpdateChase(DeltaTime);
        break;
    case EAIBehaviorState::Attack:
        UpdateAttack(DeltaTime);
        break;
    case EAIBehaviorState::Strafe:
        UpdateStrafe(DeltaTime);
        break;
    case EAIBehaviorState::Retreat:
        UpdateRetreat(DeltaTime);
        break;
    case EAIBehaviorState::Stagger:
        UpdateStagger(DeltaTime);
        break;
    default:
        break;
    }
}

void AEnemyAIController::OnMoveCompleted(bool bSuccess)
{
    // 추적 중이었다면 공격 시도
    if (BehaviorState == EAIBehaviorState::Chase && bSuccess)
    {
        if (IsTargetInAttackRange())
        {
            RequestAttack();
        }
    }
}

// ============================================================================
// 상태별 업데이트
// ============================================================================

void AEnemyAIController::UpdateIdle(float DeltaTime)
{
    // 플레이어 감지 시도
    if (TryDetectPlayer())
    {
        SetBehaviorState(EAIBehaviorState::Chase);
    }
}

void AEnemyAIController::UpdatePatrol(float DeltaTime)
{
    // 플레이어 감지 시
    if (TryDetectPlayer())
    {
        SetBehaviorState(EAIBehaviorState::Chase);
        return;
    }

    // TODO: 순찰 로직 구현
}

void AEnemyAIController::UpdateChase(float DeltaTime)
{
    // 타겟 유효성 체크
    if (!IsTargetValid())
    {
        SetBehaviorState(EAIBehaviorState::Idle);
        TargetActor = nullptr;
        return;
    }

    // 타겟을 잃었는지 확인
    if (HasLostTarget())
    {
        SetBehaviorState(EAIBehaviorState::Idle);
        TargetActor = nullptr;
        return;
    }

    // 공격 범위 안이면 공격
    if (IsTargetInAttackRange())
    {
        StopMovement();

        if (AttackCooldownTimer <= 0.f)
        {
            SetBehaviorState(EAIBehaviorState::Attack);
            int32 Pattern = SelectAttackPattern();
            ExecuteAttack(Pattern);
        }
        else
        {
            // 쿨타임 중이면 좌우 이동
            float Rand = static_cast<float>(rand()) / RAND_MAX;
            if (Rand < StrafeChance)
            {
                SetBehaviorState(EAIBehaviorState::Strafe);
            }
        }
        return;
    }

    // 타겟 방향으로 이동
    FocusOnActor(TargetActor);
    MoveToActor(TargetActor, AttackRange * 0.8f);
}

void AEnemyAIController::UpdateAttack(float DeltaTime)
{
    // 공격 중에는 타겟 방향 바라보기
    if (TargetActor)
    {
        FocusOnActor(TargetActor);
    }

    // 공격 처리 요청이 있으면 실행
    if (bAttackRequested)
    {
        bAttackRequested = false;
        int32 Pattern = SelectAttackPattern();
        ExecuteAttack(Pattern);
    }

    // TODO: 애니메이션 완료 체크
    // 임시로 즉시 완료 처리
    // OnAttackFinished();
}

void AEnemyAIController::UpdateStrafe(float DeltaTime)
{
    if (!IsTargetValid())
    {
        SetBehaviorState(EAIBehaviorState::Idle);
        return;
    }

    StrafeTimer -= DeltaTime;

    if (StrafeTimer <= 0.f)
    {
        // 다시 추적 또는 공격
        if (IsTargetInAttackRange() && AttackCooldownTimer <= 0.f)
        {
            SetBehaviorState(EAIBehaviorState::Attack);
        }
        else
        {
            SetBehaviorState(EAIBehaviorState::Chase);
        }
        return;
    }

    // 타겟을 바라보면서 좌우 이동
    FocusOnActor(TargetActor);

    if (Pawn && TargetActor)
    {
        FVector ToTarget = TargetActor->GetActorLocation() - Pawn->GetActorLocation();
        ToTarget.Z = 0.f;
        ToTarget = ToTarget.GetNormalized();

        // 수직 방향 (좌우)
        FVector StrafeDir = FVector(-ToTarget.Y, ToTarget.X, 0.f) * StrafeDirection;

        FVector NewLocation = Pawn->GetActorLocation() + StrafeDir * MoveSpeed * 0.5f * DeltaTime;
        Pawn->SetActorLocation(NewLocation);
    }
}

void AEnemyAIController::UpdateRetreat(float DeltaTime)
{
    // TODO: 후퇴 로직
    SetBehaviorState(EAIBehaviorState::Chase);
}

void AEnemyAIController::UpdateStagger(float DeltaTime)
{
    StaggerTimer -= DeltaTime;

    if (StaggerTimer <= 0.f)
    {
        // 경직 해제 후 추적 재개
        if (IsTargetValid())
        {
            SetBehaviorState(EAIBehaviorState::Chase);
        }
        else
        {
            SetBehaviorState(EAIBehaviorState::Idle);
        }
    }
}

// ============================================================================
// 감지
// ============================================================================

bool AEnemyAIController::TryDetectPlayer()
{
    if (!Pawn)
    {
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    // 플레이어 찾기
    APlayerController* PC = World->FindActor<APlayerController>();
    if (!PC || !PC->GetPawn())
    {
        return false;
    }

    APawn* PlayerPawn = PC->GetPawn();
    float Distance = GetDistanceToActor(PlayerPawn);

    if (Distance <= DetectionRange)
    {
        TargetActor = PlayerPawn;
        return true;
    }

    return false;
}

bool AEnemyAIController::IsTargetValid() const
{
    return TargetActor != nullptr;
}

bool AEnemyAIController::IsTargetInAttackRange() const
{
    if (!TargetActor)
    {
        return false;
    }

    float Distance = GetDistanceToActor(TargetActor);
    return Distance <= AttackRange;
}

bool AEnemyAIController::HasLostTarget() const
{
    if (!TargetActor)
    {
        return true;
    }

    float Distance = GetDistanceToActor(TargetActor);
    return Distance > LoseTargetRange;
}

// ============================================================================
// 공격
// ============================================================================

int32 AEnemyAIController::SelectAttackPattern()
{
    // 간단한 랜덤 선택 (같은 패턴 연속 방지)
    int32 Pattern;
    do
    {
        Pattern = rand() % MaxAttackPatterns;
    } while (Pattern == LastAttackPattern && MaxAttackPatterns > 1);

    LastAttackPattern = Pattern;
    return Pattern;
}

void AEnemyAIController::ExecuteAttack(int32 PatternIndex)
{
    AEnemyBase* Enemy = GetEnemyPawn();
    if (!Enemy)
    {
        OnAttackFinished();
        return;
    }

    // EnemyBase의 공격 실행
    Enemy->ExecuteAttackPattern(PatternIndex);

    // 쿨타임 시작
    AttackCooldownTimer = AttackCooldown;
}

void AEnemyAIController::OnAttackFinished()
{
    bIsAttacking = false;

    // 공격 후 행동 결정
    if (IsTargetValid())
    {
        if (IsTargetInAttackRange())
        {
            // 좌우 이동 또는 다시 공격 대기
            float Rand = static_cast<float>(rand()) / RAND_MAX;
            if (Rand < StrafeChance)
            {
                SetBehaviorState(EAIBehaviorState::Strafe);
            }
            else
            {
                SetBehaviorState(EAIBehaviorState::Chase);
            }
        }
        else
        {
            SetBehaviorState(EAIBehaviorState::Chase);
        }
    }
    else
    {
        SetBehaviorState(EAIBehaviorState::Idle);
    }
}

// ============================================================================
// 유틸리티
// ============================================================================

AEnemyBase* AEnemyAIController::GetEnemyPawn() const
{
    return dynamic_cast<AEnemyBase*>(Pawn);
}
