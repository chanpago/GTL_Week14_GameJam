#include "pch.h"
#include "PlayerCharacter.h"
#include "StatsComponent.h"
#include "HitboxComponent.h"
#include "SpringArmComponent.h"
#include "CameraComponent.h"
#include "InputManager.h"
#include "SkeletalMeshComponent.h"
#include "CharacterMovementComponent.h"
#include "World.h"
#include "GameModeBase.h"
#include "GameState.h"
#include "Source/Runtime/Engine/Animation/AnimMontage.h"
#include "Source/Runtime/Engine/Animation/AnimInstance.h"
#include "Source/Runtime/Engine/Animation/AnimSequence.h"
#include "Source/Runtime/AssetManagement/ResourceManager.h"



APlayerCharacter::APlayerCharacter()
{
    // 스탯 컴포넌트
    StatsComponent = CreateDefaultSubobject<UStatsComponent>("StatsComponent");

    // 히트박스 컴포넌트 (무기에 붙일 수도 있음)
    HitboxComponent = CreateDefaultSubobject<UHitboxComponent>("HitboxComponent");
    HitboxComponent->SetBoxExtent(FVector(50.f, 50.f, 50.f));

    // 스프링암 + 카메라 (필요시)
    // SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
    // Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
}

void APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    // 델리게이트 바인딩
    if (StatsComponent)
    {
        // StatsComponent->OnHealthChanged.AddDynamic(this, &APlayerCharacter::HandleHealthChanged);
        // StatsComponent->OnStaminaChanged.AddDynamic(this, &APlayerCharacter::HandleStaminaChanged);
        // StatsComponent->OnDeath.AddDynamic(this, &APlayerCharacter::HandleDeath);
    }

    // 히트박스 소유자 설정
    if (HitboxComponent)
    {
        HitboxComponent->SetOwnerActor(this);
    }

    // 공격 몽타주 초기화
    if (!LightAttackAnimPath.empty())
    {
        // AnimSequence는 FBXLoader에서 미리 로드되어 ResourceManager에 등록됨
        UAnimSequence* AttackAnim = UResourceManager::GetInstance().Get<UAnimSequence>(LightAttackAnimPath);
        if (AttackAnim)
        {
            LightAttackMontage = NewObject<UAnimMontage>();
            LightAttackMontage->SetSourceSequence(AttackAnim);
            UE_LOG("[PlayerCharacter] LightAttackMontage initialized: %s", LightAttackAnimPath.c_str());
        }
        else
        {
            UE_LOG("[PlayerCharacter] Failed to find animation: %s", LightAttackAnimPath.c_str());
        }
    }

    // 구르기 몽타주 초기화 (8방향)
    FString* DodgePaths[8] = {
        &DodgeAnimPath_F, &DodgeAnimPath_FR, &DodgeAnimPath_R, &DodgeAnimPath_BR,
        &DodgeAnimPath_B, &DodgeAnimPath_BL, &DodgeAnimPath_L, &DodgeAnimPath_FL
    };
    const char* DodgeNames[8] = { "F", "FR", "R", "BR", "B", "BL", "L", "FL" };

    for (int32 i = 0; i < 8; ++i)
    {
        if (!DodgePaths[i]->empty())
        {
            UAnimSequence* DodgeAnim = UResourceManager::GetInstance().Get<UAnimSequence>(*DodgePaths[i]);
            if (DodgeAnim)
            {
                DodgeMontages[i] = NewObject<UAnimMontage>();
                DodgeMontages[i]->SetSourceSequence(DodgeAnim);
                UE_LOG("[PlayerCharacter] DodgeMontage[%s] initialized: %s", DodgeNames[i], DodgePaths[i]->c_str());
            }
            else
            {
                UE_LOG("[PlayerCharacter] Failed to find dodge animation: %s", DodgePaths[i]->c_str());
            }
        }
    }
}

void APlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // Only process gameplay when in Fighting state
    if (AGameModeBase* GM = GWorld ? GWorld->GetGameMode() : nullptr)
    {
        if (AGameState* GS = Cast<AGameState>(GM->GetGameState()))
        {
            if (GS->GetGameFlowState() != EGameFlowState::Fighting)
            {
                return;
            }
        }
    }

    // 사망 상태면 입력 무시
    if (CombatState == ECombatState::Dead)
    {
        return;
    }

    // 공격 상태일 때 몽타주 종료 체크
    UpdateAttackState(DeltaSeconds);

    // 구르기 상태 업데이트
    UpdateDodgeState(DeltaSeconds);

    // 경직 상태 업데이트
    UpdateStagger(DeltaSeconds);

    // 패리 윈도우 업데이트
    UpdateParryWindow(DeltaSeconds);

    // 경직 중이 아니면 입력 처리
    if (CombatState != ECombatState::Staggered)
    {
        ProcessInput(DeltaSeconds);
    }
}

// ============================================================================
// 입력 처리
// ============================================================================

void APlayerCharacter::ProcessInput(float DeltaTime)
{
  //  ProcessMovementInput(DeltaTime);
    ProcessCombatInput();
}

void APlayerCharacter::ProcessMovementInput(float DeltaTime)
{
    // 공격/회피 중에는 이동 불가
    if (CombatState == ECombatState::Attacking || CombatState == ECombatState::Dodging)
    {
        return;
    }

    FVector MoveDirection = FVector();

    if (INPUT.IsKeyDown('W')) MoveDirection.Y += 1.f;
    if (INPUT.IsKeyDown('S')) MoveDirection.Y -= 1.f;
    if (INPUT.IsKeyDown('A')) MoveDirection.X -= 1.f;
    if (INPUT.IsKeyDown('D')) MoveDirection.X += 1.f;

    if (MoveDirection.SizeSquared() > 0.f)
    {
        MoveDirection.Normalize();
        FVector NewLocation = GetActorLocation() + MoveDirection * MoveSpeed * DeltaTime;
        SetActorLocation(NewLocation);

        // 이동 방향으로 회전
        // FQuat TargetRotation = FQuat::LookAt(MoveDirection, FVector::UpVector);
        // SetActorRotation(TargetRotation);
    }
}

void APlayerCharacter::ProcessCombatInput()
{
    // 마우스 좌클릭 - 약공격
    if (INPUT.IsMouseButtonPressed(LeftButton))
    {
        LightAttack();
    }

    // 마우스 우클릭 - 가드
    if (INPUT.IsMouseButtonPressed(RightButton))
    {
        StartBlock();
    }
    if (INPUT.IsMouseButtonReleased(RightButton))
    {
        StopBlock();
    }

    // 스페이스 - 회피
    if (INPUT.IsKeyPressed(VK_SPACE))
    {
        Dodge();
    }

    // Shift + 좌클릭 - 강공격
    if (INPUT.IsKeyDown(VK_SHIFT) && INPUT.IsMouseButtonPressed(LeftButton))
    {
        HeavyAttack();
    }
}

// ============================================================================
// 전투 액션
// ============================================================================

void APlayerCharacter::LightAttack()
{
    // 상태 체크
    if (CombatState == ECombatState::Attacking && !bCanCombo)
    {
        return;
    }
    if (CombatState == ECombatState::Staggered || CombatState == ECombatState::Dead)
    {
        return;
    }

    // 스태미나 체크
    if (!StatsComponent->ConsumeStamina(StatsComponent->LightAttackCost))
    {
        return; // 스태미나 부족
    }

    // 가드 해제
    StopBlock();

    SetCombatState(ECombatState::Attacking);

    // 콤보 카운트 증가
    if (bCanCombo)
    {
        ComboCount = (ComboCount + 1) % MaxComboCount;
    }
    else
    {
        ComboCount = 0;
    }

    // 히트박스 활성화
    FDamageInfo DamageInfo(this, 10.f + ComboCount * 5.f, EDamageType::Light);
    HitboxComponent->EnableHitbox(DamageInfo);

    // 공격 애니메이션 재생 (몽타주)
    if (LightAttackMontage)
    {
        if (USkeletalMeshComponent* Mesh = GetMesh())
        {
            if (UAnimInstance* AnimInst = Mesh->GetAnimInstance())
            {
                // 루트 모션 활성화 및 애니메이션 끝 자르기 시간 설정
                AnimInst->SetRootMotionEnabled(bEnableAttackRootMotion);
                AnimInst->SetAnimationCutEndTime(AnimationCutEndTime);

                AnimInst->Montage_Play(LightAttackMontage, 0.1f, 0.01f, 1.0f);  // BlendOut 최소화
                UE_LOG("[PlayerCharacter] Playing LightAttack montage (RootMotion: %s, CutEndTime: %.3fs)",
                    bEnableAttackRootMotion ? "ON" : "OFF", AnimationCutEndTime);
            }
        }
    }

    bCanCombo = false;
}

void APlayerCharacter::HeavyAttack()
{
    if (CombatState == ECombatState::Attacking ||
        CombatState == ECombatState::Staggered ||
        CombatState == ECombatState::Dead)
    {
        return;
    }

    if (!StatsComponent->ConsumeStamina(StatsComponent->HeavyAttackCost))
    {
        return;
    }

    StopBlock();
    SetCombatState(ECombatState::Attacking);
    ComboCount = 0;

    FDamageInfo DamageInfo(this, 30.f, EDamageType::Heavy);
    DamageInfo.HitReaction = EHitReaction::Stagger;
    DamageInfo.StaggerDuration = 0.5f;
    HitboxComponent->EnableHitbox(DamageInfo);

    // TODO: 강공격 애니메이션 재생
}

void APlayerCharacter::Dodge()
{
    if (CombatState == ECombatState::Dodging ||
        CombatState == ECombatState::Staggered ||
        CombatState == ECombatState::Dead)
    {
        return;
    }

    if (!StatsComponent->ConsumeStamina(StatsComponent->DodgeCost))
    {
        return;
    }

    StopBlock();
    SetCombatState(ECombatState::Dodging);
    bIsInvincible = true;

    // 입력 방향에 따라 8방향 구르기 몽타주 선택
    int32 DodgeIndex = GetDodgeDirectionIndex();
    UAnimMontage* SelectedMontage = DodgeMontages[DodgeIndex];

    if (SelectedMontage)
    {
        if (USkeletalMeshComponent* Mesh = GetMesh())
        {
            if (UAnimInstance* AnimInst = Mesh->GetAnimInstance())
            {
                // 루트 모션 활성화 및 애니메이션 끝 자르기 시간 설정
                AnimInst->SetRootMotionEnabled(bEnableDodgeRootMotion);
                AnimInst->SetAnimationCutEndTime(DodgeAnimationCutEndTime);

                AnimInst->Montage_Play(SelectedMontage, 0.1f, 0.1f, 1.0f);
                const char* DirNames[8] = { "F", "FR", "R", "BR", "B", "BL", "L", "FL" };
                UE_LOG("[PlayerCharacter] Playing Dodge montage: %s (RootMotion: %s, CutEndTime: %.3fs)",
                    DirNames[DodgeIndex], bEnableDodgeRootMotion ? "ON" : "OFF", DodgeAnimationCutEndTime);
            }
        }
    }
    else
    {
        // 몽타주가 없으면 기본 Forward로 폴백
        if (DodgeMontages[0])
        {
            if (USkeletalMeshComponent* Mesh = GetMesh())
            {
                if (UAnimInstance* AnimInst = Mesh->GetAnimInstance())
                {
                    // 루트 모션 활성화 및 애니메이션 끝 자르기 시간 설정
                    AnimInst->SetRootMotionEnabled(bEnableDodgeRootMotion);
                    AnimInst->SetAnimationCutEndTime(DodgeAnimationCutEndTime);

                    AnimInst->Montage_Play(DodgeMontages[0], 0.1f, 0.1f, 1.0f);
                    UE_LOG("[PlayerCharacter] Playing Dodge montage: F (fallback, RootMotion: %s, CutEndTime: %.3fs)",
                        bEnableDodgeRootMotion ? "ON" : "OFF", DodgeAnimationCutEndTime);
                }
            }
        }
    }
}

void APlayerCharacter::StartBlock()
{
    if (CombatState == ECombatState::Attacking ||
        CombatState == ECombatState::Dodging ||
        CombatState == ECombatState::Dead)
    {
        return;
    }

    bIsBlocking = true;
    SetCombatState(ECombatState::Blocking);

    // 패리 윈도우 시작
    bIsParrying = true;
    ParryWindowTimer = ParryWindowDuration;

    // TODO: 가드 애니메이션 재생
}

void APlayerCharacter::StopBlock()
{
    if (!bIsBlocking)
    {
        return;
    }

    bIsBlocking = false;
    bIsParrying = false;
    ParryWindowTimer = 0.f;

    if (CombatState == ECombatState::Blocking)
    {
        SetCombatState(ECombatState::Idle);
    }
}

// ============================================================================
// IDamageable 구현
// ============================================================================

float APlayerCharacter::TakeDamage(const FDamageInfo& DamageInfo)
{
    if (!CanBeHit())
    {
        return 0.f;
    }

    float ActualDamage = DamageInfo.Damage;

    // 가드 중이면 데미지 감소
    if (bIsBlocking && DamageInfo.bCanBeBlocked)
    {
        ActualDamage *= 0.2f; // 80% 감소
        StatsComponent->ConsumeStamina(StatsComponent->BlockCostPerHit);
    }

    StatsComponent->ApplyDamage(ActualDamage);

    // 피격 반응 (가드 중이 아니거나 슈퍼아머가 아니면)
    if (!bIsBlocking)
    {
        OnHitReaction(DamageInfo.HitReaction, DamageInfo);
    }

    return ActualDamage;
}

bool APlayerCharacter::IsAlive() const
{
    return StatsComponent && StatsComponent->IsAlive();
}

bool APlayerCharacter::CanBeHit() const
{
    return IsAlive() && !bIsInvincible;
}

bool APlayerCharacter::IsBlocking() const
{
    return bIsBlocking;
}

bool APlayerCharacter::IsParrying() const
{
    return bIsParrying;
}

void APlayerCharacter::OnHitReaction(EHitReaction Reaction, const FDamageInfo& DamageInfo)
{
    if (Reaction == EHitReaction::None)
    {
        return;
    }

    // 현재 공격/회피 중단
    HitboxComponent->DisableHitbox();
    bIsInvincible = false;

    SetCombatState(ECombatState::Staggered);
    StaggerTimer = DamageInfo.StaggerDuration;

    // TODO: 피격 애니메이션 재생
    // PlayHitReactionAnimation(Reaction);

    // 넉백 적용
    if (Reaction == EHitReaction::Knockback && DamageInfo.KnockbackForce > 0.f)
    {
        FVector KnockbackDir = DamageInfo.HitDirection * DamageInfo.KnockbackForce;
        AddActorWorldLocation(KnockbackDir * 0.1f); // 간단한 넉백
    }
}

void APlayerCharacter::OnDeath()
{
    SetCombatState(ECombatState::Dead);
    HitboxComponent->DisableHitbox();

    // TODO: 사망 애니메이션/래그돌
}

// ============================================================================
// 내부 함수
// ============================================================================

void APlayerCharacter::SetCombatState(ECombatState NewState)
{
    ECombatState OldState = CombatState;
    CombatState = NewState;

    // 상태 변경 시 처리
    if (OldState == ECombatState::Attacking && NewState != ECombatState::Attacking)
    {
        HitboxComponent->DisableHitbox();
        HitboxComponent->ClearHitActors();
    }
}

void APlayerCharacter::UpdateParryWindow(float DeltaTime)
{
    if (bIsParrying && ParryWindowTimer > 0.f)
    {
        ParryWindowTimer -= DeltaTime;
        if (ParryWindowTimer <= 0.f)
        {
            bIsParrying = false;
        }
    }
}

void APlayerCharacter::UpdateAttackState(float DeltaTime)
{
    // 공격 상태일 때만 체크
    if (CombatState != ECombatState::Attacking)
    {
        return;
    }

    // 몽타주가 끝났는지 확인
    if (USkeletalMeshComponent* Mesh = GetMesh())
    {
        if (UAnimInstance* AnimInst = Mesh->GetAnimInstance())
        {
            // 몽타주가 재생 중이 아니면 공격 종료
            if (!AnimInst->Montage_IsPlaying())
            {
                SetCombatState(ECombatState::Idle);
                UE_LOG("[PlayerCharacter] Attack finished, returning to Idle");
            }
        }
    }
}

void APlayerCharacter::UpdateStagger(float DeltaTime)
{
    if (CombatState == ECombatState::Staggered && StaggerTimer > 0.f)
    {
        StaggerTimer -= DeltaTime;
        if (StaggerTimer <= 0.f)
        {
            SetCombatState(ECombatState::Idle);
        }
    }
}

// ============================================================================
// 콜백
// ============================================================================

void APlayerCharacter::HandleHealthChanged(float CurrentHealth, float MaxHealth)
{
    // UI 업데이트는 UI 클래스에서 델리게이트 바인딩해서 처리
}

void APlayerCharacter::HandleStaminaChanged(float CurrentStamina, float MaxStamina)
{
    // UI 업데이트는 UI 클래스에서 델리게이트 바인딩해서 처리
}

void APlayerCharacter::HandleDeath()
{
    OnDeath();
}

int32 APlayerCharacter::GetDodgeDirectionIndex() const
{
    // 캐릭터의 실제 이동 속도(Local Velocity) 기준으로 방향 결정
    FVector LocalVel = FVector::Zero();

    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        // 월드 Velocity를 캐릭터 로컬 좌표계로 변환
        FVector WorldVel = Movement->GetVelocity();
        WorldVel.Z = 0.0f;  // 수평 방향만 고려

        // 캐릭터 회전의 역방향으로 회전시켜 로컬 좌표계로 변환
        FQuat ActorRot = GetActorRotation();
        LocalVel = ActorRot.Inverse().RotateVector(WorldVel);
    }

    // 속도가 없으면 Forward (0)
    if (LocalVel.IsZero() || LocalVel.Size() < 0.1f)
    {
        return 0;
    }

    // 각도 계산 (atan2: Y, X 순서) - 결과는 -PI ~ PI
    // Forward(+X) = 0도, Right(+Y) = 90도, Backward(-X) = 180도, Left(-Y) = -90도
    float AngleRad = std::atan2(LocalVel.Y, LocalVel.X);
    float AngleDeg = AngleRad * (180.0f / PI);

    // 8방향 인덱스 계산 (각 방향은 45도 간격)
    // 0=F(0°), 1=FR(45°), 2=R(90°), 3=BR(135°), 4=B(180°/-180°), 5=BL(-135°), 6=L(-90°), 7=FL(-45°)

    // 각도를 0~360 범위로 변환
    if (AngleDeg < 0) AngleDeg += 360.0f;

    // 22.5도 오프셋을 더해서 각 방향의 중심이 경계가 되도록 함
    AngleDeg += 22.5f;
    if (AngleDeg >= 360.0f) AngleDeg -= 360.0f;

    // 45도 단위로 나눠서 인덱스 계산
    int32 Index = static_cast<int32>(AngleDeg / 45.0f);
    Index = FMath::Clamp(Index, 0, 7);

    return Index;
}

void APlayerCharacter::UpdateDodgeState(float DeltaTime)
{
    // 구르기 상태일 때만 체크
    if (CombatState != ECombatState::Dodging)
    {
        return;
    }

    // 몽타주가 끝났는지 확인
    if (USkeletalMeshComponent* Mesh = GetMesh())
    {
        if (UAnimInstance* AnimInst = Mesh->GetAnimInstance())
        {
            // 몽타주가 재생 중이 아니면 구르기 종료
            if (!AnimInst->Montage_IsPlaying())
            {
                bIsInvincible = false;
                SetCombatState(ECombatState::Idle);
                UE_LOG("[PlayerCharacter] Dodge finished, returning to Idle");
            }
        }
    }
}
