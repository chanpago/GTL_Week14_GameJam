#include "pch.h"
#include "AIController.h"
#include "Pawn.h"
#include "Actor.h"

AAIController::AAIController()
{
}

void AAIController::BeginPlay()
{
    Super::BeginPlay();
}

void AAIController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    //if (!bAIEnabled || !Pawn)
    //{
    //    return;
    //}

    //// 이동 처리
    //if (bIsMoving)
    //{
    //    UpdateMovement(DeltaSeconds);
    //}

    //// 회전 처리
    //if (bHasFocusPoint)
    //{
    //    UpdateRotation(DeltaSeconds);
    //}

    //// AI 로직 업데이트
    //UpdateAI(DeltaSeconds);
}

// ============================================================================
// 이동
// ============================================================================

void AAIController::MoveToActor(AActor* Goal, float InAcceptanceRadius)
{
    if (!Goal)
    {
        StopMovement();
        return;
    }

    MoveDestination = Goal->GetActorLocation();
    AcceptanceRadius = InAcceptanceRadius;
    bIsMoving = true;
    bHasMoveGoal = true;
}

void AAIController::MoveToLocation(const FVector& Location, float InAcceptanceRadius)
{
    MoveDestination = Location;
    AcceptanceRadius = InAcceptanceRadius;
    bIsMoving = true;
    bHasMoveGoal = true;
}

void AAIController::StopMovement()
{
    bIsMoving = false;
    bHasMoveGoal = false;
}

void AAIController::UpdateMovement(float DeltaTime)
{
    if (!Pawn || !bHasMoveGoal)
    {
        return;
    }

    FVector CurrentLocation = Pawn->GetActorLocation();
    FVector Direction = MoveDestination - CurrentLocation;
    Direction.Z = 0.f; // 수평 이동만

    float Distance = Direction.Size();

    // 도착 체크
    if (Distance <= AcceptanceRadius)
    {
        StopMovement();
        OnMoveCompleted(true);
        return;
    }

    // 이동
    Direction = Direction.GetNormalized();
    FVector NewLocation = CurrentLocation + Direction * MoveSpeed * DeltaTime;
    Pawn->SetActorLocation(NewLocation);
}

// ============================================================================
// 회전
// ============================================================================

void AAIController::FocusOnActor(AActor* Actor)
{
    if (!Actor)
    {
        bHasFocusPoint = false;
        return;
    }

    FocusLocation = Actor->GetActorLocation();
    bHasFocusPoint = true;
}

void AAIController::FocusOnLocation(const FVector& Location)
{
    FocusLocation = Location;
    bHasFocusPoint = true;
}

void AAIController::UpdateRotation(float DeltaTime)
{
    if (!Pawn || !bHasFocusPoint)
    {
        return;
    }

    FVector CurrentLocation = Pawn->GetActorLocation();
    FVector Direction = FocusLocation - CurrentLocation;
    Direction.Z = 0.f;

    if (Direction.SizeSquared() < 0.01f)
    {
        return;
    }

    Direction = Direction.GetNormalized();

    // 목표 회전 계산
    FQuat TargetRotation = FQuat::FindBetweenVectors(FVector(1.f, 0.f, 0.f), Direction);

    // 부드러운 회전 보간
    FQuat CurrentRotation = Pawn->GetActorRotation();
    FQuat NewRotation = FQuat::Slerp(CurrentRotation, TargetRotation, RotationSpeed * DeltaTime);

    Pawn->SetActorRotation(NewRotation);
}

// ============================================================================
// 거리 계산
// ============================================================================

float AAIController::GetDistanceToActor(AActor* Actor) const
{
    if (!Pawn || !Actor)
    {
        return FLT_MAX;
    }

    return (Actor->GetActorLocation() - Pawn->GetActorLocation()).Size();
}

float AAIController::GetDistanceToLocation(const FVector& Location) const
{
    if (!Pawn)
    {
        return FLT_MAX;
    }

    return (Location - Pawn->GetActorLocation()).Size();
}

bool AAIController::HasReachedDestination() const
{
    if (!Pawn || !bHasMoveGoal)
    {
        return true;
    }

    float Distance = (MoveDestination - Pawn->GetActorLocation()).Size();
    return Distance <= AcceptanceRadius;
}

// ============================================================================
// AI 로직 (서브클래스에서 오버라이드)
// ============================================================================

void AAIController::UpdateAI(float DeltaTime)
{
    // 베이스 클래스에서는 아무것도 안 함
    // 서브클래스에서 구현
}

void AAIController::OnMoveCompleted(bool bSuccess)
{
    // 서브클래스에서 오버라이드
}
