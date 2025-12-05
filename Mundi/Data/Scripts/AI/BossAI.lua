-- ============================================================================
-- BossAI.lua - 보스 전용 AI (Behavior Tree 기반)
-- ============================================================================

-- BT 프레임워크 로드
dofile("Data/Scripts/AI/BehaviorTree.lua")

-- ============================================================================
-- 디버그 설정
-- ============================================================================
local DEBUG_LOG = true  -- false로 바꾸면 로그 끔

local function Log(msg)
    if DEBUG_LOG then
        print("[BossAI] " .. msg)
    end
end

-- ============================================================================
-- Context (AI 상태 저장)
-- ============================================================================

local ctx = {
    self_actor = nil,
    target = nil,
    stats = nil,
    phase = 1,
    time = 0,
    is_attacking = false,
    attack_end_time = 0,
    last_pattern = -1
}

-- ============================================================================
-- 설정값
-- ============================================================================

local Config = {
    DetectionRange = 2000,
    AttackRange = 10,
    LoseTargetRange = 3000,
    MoveSpeed = 250,
    Phase2MoveSpeed = 350,
    Phase2HealthThreshold = 0.5,

    -- 공격 쿨다운
    LightAttackCooldown = 1.5,
    HeavyAttackCooldown = 3.0,
    ChargeAttackCooldown = 4.0,
    SpinAttackCooldown = 5.0,

    -- 공격 지속 시간
    LightAttackDuration = 0.8,
    HeavyAttackDuration = 1.2,
    ChargeAttackDuration = 1.0,
    SpinAttackDuration = 1.5
}

-- ============================================================================
-- Conditions (조건 함수들)
-- ============================================================================

local function HasTarget(c)
    local result = c.target ~= nil
    Log("  [Cond] HasTarget: " .. tostring(result))
    return result
end

local function IsTargetInDetectionRange(c)
    if not c.target or not c.self_actor then
        Log("  [Cond] IsTargetInDetectionRange: false (no target)")
        return false
    end
    local dist = VecDistance(Obj.Location, c.target.Location)
    local result = dist <= Config.DetectionRange
    Log("  [Cond] IsTargetInDetectionRange: " .. tostring(result) .. " (dist=" .. string.format("%.1f", dist) .. ")")
    return result
end

local function IsTargetInAttackRange(c)
    if not c.target or not c.self_actor then
        Log("  [Cond] IsTargetInAttackRange: false (no target)")
        return false
    end
    local dist = VecDistance(Obj.Location, c.target.Location)
    local result = dist <= Config.AttackRange
    Log("  [Cond] IsTargetInAttackRange: " .. tostring(result) .. " (dist=" .. string.format("%.1f", dist) .. ", range=" .. Config.AttackRange .. ")")
    return result
end

local function IsTargetLost(c)
    if not c.target or not c.self_actor then
        Log("  [Cond] IsTargetLost: true (no target)")
        return true
    end
    local dist = VecDistance(Obj.Location, c.target.Location)
    local result = dist > Config.LoseTargetRange
    Log("  [Cond] IsTargetLost: " .. tostring(result) .. " (dist=" .. string.format("%.1f", dist) .. ")")
    return result
end

local function IsPhase2(c)
    local result = c.phase >= 2
    Log("  [Cond] IsPhase2: " .. tostring(result) .. " (phase=" .. c.phase .. ")")
    return result
end

local function IsLowHealth(c)
    if not c.stats then
        Log("  [Cond] IsLowHealth: false (no stats)")
        return false
    end
    local healthPercent = c.stats.CurrentHealth / c.stats.MaxHealth
    local result = healthPercent < 0.3
    Log("  [Cond] IsLowHealth: " .. tostring(result) .. " (hp=" .. string.format("%.1f%%", healthPercent * 100) .. ")")
    return result
end

local function IsNotAttacking(c)
    local result = not c.is_attacking
    Log("  [Cond] IsNotAttacking: " .. tostring(result))
    return result
end

local function IsAttacking(c)
    local result = c.is_attacking
    Log("  [Cond] IsAttacking: " .. tostring(result))
    return result
end

-- ============================================================================
-- Actions (행동 함수들)
-- ============================================================================

local function DoFindTarget(c)
    Log("  [Action] DoFindTarget")
    -- GetPlayer() 우선 시도, 없으면 FindObjectByName 시도
    c.target = GetPlayer()
    if c.target then
        Log("    -> Target found via GetPlayer()")
        return BT_SUCCESS
    end
    c.target = FindObjectByName("Player")
    if c.target then
        Log("    -> Target found via FindObjectByName")
        return BT_SUCCESS
    end
    Log("    -> Target not found")
    return BT_FAILURE
end

local function DoLoseTarget(c)
    Log("  [Action] DoLoseTarget")
    c.target = nil
    return BT_SUCCESS
end

local function DoChase(c)
    if not c.target then
        Log("  [Action] DoChase -> FAILURE (no target)")
        return BT_FAILURE
    end

    -- FVector를 Lua 테이블로 변환
    local myPosRaw = Obj.Location
    local targetPosRaw = c.target.Location
    local myPos = { X = myPosRaw.X, Y = myPosRaw.Y, Z = myPosRaw.Z }
    local targetPos = { X = targetPosRaw.X, Y = targetPosRaw.Y, Z = targetPosRaw.Z }

    local dir = VecDirection(myPos, targetPos)
    dir.Z = 0  -- 수평 이동만

    local dist = VecDistance(myPos, targetPos)

    -- 타겟을 바라보도록 회전
    local yaw = CalcYaw(myPos, targetPos)
    Obj.Rotation = Vector(0, 0, yaw)

    -- CharacterMovementComponent 사용
    local moveDir = Vector(dir.X, dir.Y, dir.Z)
    AddMovementInput(Obj, moveDir, 1.0)

    Log("  [Action] DoChase -> Moving via AddMovementInput (dist=" .. string.format("%.1f", dist) .. ", yaw=" .. string.format("%.1f", yaw) .. ")")
    return BT_SUCCESS
end

local function DoIdle(c)
    Log("  [Action] DoIdle")
    return BT_SUCCESS
end

-- ============================================================================
-- Attack Actions (공격 행동들)
-- ============================================================================

local function StartAttack(c, duration, patternName)
    c.is_attacking = true
    c.attack_end_time = c.time + duration
    Log("    -> StartAttack: " .. patternName .. " (duration=" .. duration .. "s)")
end

local function DoLightAttack(c)
    Log("  [Action] DoLightAttack")
    if c.is_attacking then
        Log("    -> FAILURE (already attacking)")
        return BT_FAILURE
    end

    StartAttack(c, Config.LightAttackDuration, "LightCombo")
    c.last_pattern = 0

    local hitbox = GetComponent(Obj, "UHitboxComponent")
    if hitbox then
        hitbox:EnableHitbox(20, "Light", 0.3)
        Log("    -> Hitbox enabled (dmg=20)")
    end

    return BT_SUCCESS
end

local function DoHeavyAttack(c)
    Log("  [Action] DoHeavyAttack")
    if c.is_attacking then
        Log("    -> FAILURE (already attacking)")
        return BT_FAILURE
    end

    StartAttack(c, Config.HeavyAttackDuration, "HeavySlam")
    c.last_pattern = 1

    local hitbox = GetComponent(Obj, "UHitboxComponent")
    if hitbox then
        hitbox:EnableHitbox(40, "Heavy", 0.8)
        Log("    -> Hitbox enabled (dmg=40)")
    end

    return BT_SUCCESS
end

local function DoChargeAttack(c)
    Log("  [Action] DoChargeAttack")
    if c.is_attacking then
        Log("    -> FAILURE (already attacking)")
        return BT_FAILURE
    end
    if not c.target then
        Log("    -> FAILURE (no target)")
        return BT_FAILURE
    end

    StartAttack(c, Config.ChargeAttackDuration, "ChargeAttack")
    c.last_pattern = 2

    local dir = VecDirection(Obj.Location, c.target.Location)
    dir.Z = 0
    Obj.Location = Vector(
        Obj.Location.X + dir.X * 500,
        Obj.Location.Y + dir.Y * 500,
        Obj.Location.Z
    )
    Log("    -> Charged forward 500 units")

    local hitbox = GetComponent(Obj, "UHitboxComponent")
    if hitbox then
        hitbox:EnableHitbox(35, "Heavy", 0.6)
        Log("    -> Hitbox enabled (dmg=35)")
    end

    return BT_SUCCESS
end

local function DoSpinAttack(c)
    Log("  [Action] DoSpinAttack")
    if c.is_attacking then
        Log("    -> FAILURE (already attacking)")
        return BT_FAILURE
    end

    StartAttack(c, Config.SpinAttackDuration, "SpinAttack")
    c.last_pattern = 3

    local hitbox = GetComponent(Obj, "UHitboxComponent")
    if hitbox then
        hitbox:EnableHitbox(25, "Special", 0.5)
        Log("    -> Hitbox enabled (dmg=25)")
    end

    return BT_SUCCESS
end

local function DoRetreat(c)
    Log("  [Action] DoRetreat")
    if not c.target then
        Log("    -> FAILURE (no target)")
        return BT_FAILURE
    end

    local dir = VecDirection(c.target.Location, Obj.Location)
    dir.Z = 0

    local delta = GetWorld():GetDeltaSeconds()
    local retreatSpeed = 400

    Obj.Location = Vector(
        Obj.Location.X + dir.X * retreatSpeed * delta,
        Obj.Location.Y + dir.Y * retreatSpeed * delta,
        Obj.Location.Z
    )

    Log("    -> Retreating (speed=" .. retreatSpeed .. ")")
    return BT_SUCCESS
end

-- ============================================================================
-- Phase System (페이즈 시스템)
-- ============================================================================

local function CheckPhaseTransition(c)
    if not c.stats or c.phase >= 2 then return end

    local healthPercent = c.stats.CurrentHealth / c.stats.MaxHealth
    if healthPercent <= Config.Phase2HealthThreshold then
        c.phase = 2
        Log("*** PHASE TRANSITION: 1 -> 2 ***")

        local camMgr = GetCameraManager()
        if camMgr then
            camMgr:StartCameraShake(0.5, 0.3, 0.3, 30)
        end
    end
end

local function UpdateAttackState(c)
    if c.is_attacking and c.time >= c.attack_end_time then
        c.is_attacking = false
        Log("Attack ended")

        local hitbox = GetComponent(Obj, "UHitboxComponent")
        if hitbox then
            hitbox:DisableHitbox()
        end
    end
end

-- ============================================================================
-- Behavior Tree 구성
-- ============================================================================

local BossTree = Selector({
    -- 1. 타겟 소실 시 재탐색
    Sequence({
        Condition(function(c)
            local result = not HasTarget(c) or IsTargetLost(c)
            Log("[Branch 1] TargetLost? " .. tostring(result))
            return result
        end),
        Action(DoFindTarget)
    }),

    -- 2. 추적
    Sequence({
        Condition(function(c)
            Log("[Branch 2] Chase?")
            return HasTarget(c)
        end),
        Action(DoChase)
    }),

    -- 3. 기본 - 대기
    Action(function(c)
        Log("[Branch 3] Idle (fallback)")
        return DoIdle(c)
    end)
})

-- ============================================================================
-- Callbacks (엔진 콜백)
-- ============================================================================

function BeginPlay()
    Log("========== BeginPlay ==========")

    ctx.self_actor = Obj
    ctx.stats = GetComponent(Obj, "UStatsComponent")
    ctx.phase = 1
    ctx.time = 0
    ctx.is_attacking = false

    if ctx.stats then
        Log("Stats found: HP=" .. ctx.stats.CurrentHealth .. "/" .. ctx.stats.MaxHealth)
    else
        Log("WARNING: No UStatsComponent found!")
    end

    -- GetPlayer() 우선 시도
    ctx.target = GetPlayer()
    if ctx.target then
        Log("Initial target found via GetPlayer()")
    else
        ctx.target = FindObjectByName("Player")
        if ctx.target then
            Log("Initial target found via FindObjectByName")
        else
            Log("WARNING: Player not found!")
        end
    end
end

function Tick(Delta)
    ctx.time = ctx.time + Delta

    -- 페이즈 전환 체크
    CheckPhaseTransition(ctx)

    -- 공격 상태 업데이트
    UpdateAttackState(ctx)

    -- BT 실행
    BossTree(ctx)
end

function OnHit(OtherActor, HitInfo)
    local name = "Unknown"
    if OtherActor and OtherActor.Name then
        name = OtherActor.Name
    end
    Log("OnHit from: " .. name)

    if OtherActor and (OtherActor.Tag == "Player" or OtherActor.Tag == "PlayerWeapon") then
        local player = FindObjectByName("Player")
        if player then
            ctx.target = player
            Log("Target updated to attacker")
        end
    end
end

function EndPlay()
    Log("========== EndPlay ==========")
end
