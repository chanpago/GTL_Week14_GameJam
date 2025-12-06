#pragma once
#include "GameStateBase.h"
#include "AGameState.generated.h"

// Game-specific flow and UI state for the jam project
enum class EGameFlowState : uint8
{
    StartMenu,
    BossIntro,
    Fighting,
    Paused,
    Victory,
    Defeat
};

struct FHealthState
{
    float Current = 0.0f;
    float Max = 0.0f;
    float GetPercent() const { return (Max > 0.0f) ? (Current / Max) : 0.0f; }
    void Set(float InCurrent, float InMax) { Current = InCurrent; Max = InMax; }
};

struct FStaminaState
{
    float Current = 0.0f;
    float Max = 0.0f;
    float GetPercent() const { return (Max > 0.0f) ? (Current / Max) : 0.0f; }
    void Set(float InCurrent, float InMax) { Current = InCurrent; Max = InMax; }
};

struct FFocusState
{
    float Current = 0.0f;
    float Max = 0.0f;
    float GetPercent() const { return (Max > 0.0f) ? (Current / Max) : 0.0f; }
    void Set(float InCurrent, float InMax) { Current = InCurrent; Max = InMax; }
};

class AGameState : public AGameStateBase
{
public:
    GENERATED_REFLECTION_BODY()

    AGameState() {};
    ~AGameState() override = default;

    // Flow control
    EGameFlowState GetGameFlowState() const { return GameFlowState; }
    void SetGameFlowState(EGameFlowState NewState);
    void EnterStartMenu();
    void StartFight();
    void EnterBossIntro();
    void EnterVictory();
    void EnterDefeat();

    // Notifications from GameMode/Controllers
    void OnPlayerLogin(APlayerController* InController) override;
    void OnPawnPossessed(APawn* InPawn) override;
    void OnPlayerDied() override;

    // Player health updates
    void OnPlayerHealthChanged(float Current, float Max);
    const FHealthState& GetPlayerHealth() const { return PlayerHealth; }

    // Player stamina updates
    void OnPlayerStaminaChanged(float Current, float Max);
    const FStaminaState& GetPlayerStamina() const { return PlayerStamina; }

    // Player focus updates
    void OnPlayerFocusChanged(float Current, float Max);
    const FFocusState& GetPlayerFocus() const { return PlayerFocus; }

    // Boss info and health
    void RegisterBoss(const FString& InBossName, float BossMaxHealth);
    void UnregisterBoss();
    void OnBossHealthChanged(float Current);
    bool HasActiveBoss() const { return bBossActive; }
    const FString& GetBossName() const { return BossName; }
    const FHealthState& GetBossHealth() const { return BossHealth; }

    // UI toggles for overlay
    void ShowStartScreen(bool bShow);
    void ShowEndScreen(bool bShow, bool bPlayerWon);
    bool IsStartScreenVisible() const { return bStartScreenVisible; }
    bool IsEndScreenVisible() const { return bEndScreenVisible; }
    bool DidPlayerWin() const { return bPlayerWon; }

    float GetStateElapsedTime() const { return StateTimeSeconds; }

protected:
    void HandleStateTick(float DeltaTime) override;

protected:
    // Flow
    EGameFlowState GameFlowState = EGameFlowState::StartMenu;

    // Player
    FHealthState PlayerHealth;
    FStaminaState PlayerStamina;
    FFocusState PlayerFocus;
    bool bPlayerAlive = true;

    // Boss
    FString BossName;
    FHealthState BossHealth;
    bool bBossActive = false;

    // UI flags
    bool bStartScreenVisible = true;
    bool bEndScreenVisible = false;
    bool bPlayerWon = false;

    // Timings (seconds)
    float StartFadeInDuration = 0.5f;
    float EndFadeInDuration = 0.6f;
    float BossIntroBannerTime = 2.0f;
};
