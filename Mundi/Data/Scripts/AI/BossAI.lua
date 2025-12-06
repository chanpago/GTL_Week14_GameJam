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
    delta_time = 0,                 -- 이번 프레임 delta time
    last_pattern = -1,
    -- Strafe 관련
    strafe_direction = 1,           -- 1: 오른쪽, -1: 왼쪽
    strafe_last_change_time = 0,    -- 마지막 방향 전환 시간
    -- 공격 쿨다운
    last_attack_time = 0,           -- 마지막 공격 시간
    -- 몽타주 상태 추적
    was_attacking = false,          -- 이전 프레임 공격 상태 (몽타주 종료 감지용)
    -- 후퇴 관련
    is_retreating = false,          -- 후퇴 중인지
    retreat_end_time = 0            -- 후퇴 종료 시간
}

-- ============================================================================
-- 설정값
-- ============================================================================

local Config = {
    DetectionRange = 2000,
    AttackRange = 3,            -- 3m 이내면 공격
    LoseTargetRange = 3000,
    MoveSpeed = 250,
    Phase2MoveSpeed = 350,
    Phase2HealthThreshold = 0.5,

    -- 좌우 이동(Strafe) 설정 (미터 단위)
    StrafeMinRange = 4,         -- 4m 이상일 때 좌우 이동 시작 (AttackRange보다 커야함)
    StrafeMaxRange = 8,         -- 8m 이하일 때 좌우 이동
    StrafeSpeed = 2,            -- 좌우 이동 속도 (m/s)
    StrafeChangeInterval = 1.5, -- 방향 전환 간격 (초)

    -- 후퇴 설정 (미터 단위)
    RetreatRange = 2,           -- 이 거리보다 가까우면 후퇴 (AttackRange보다 작아야함)
    RetreatChance = 0.3,        -- 공격 후 후퇴할 확률 (0~1)
    RetreatDuration = 1.0,      -- 후퇴 지속 시간 (초)

    -- 공격 쿨다운 (초)
    AttackCooldown = 2.0,       -- 공격 후 다음 공격까지 대기 시간

    -- 회전 보간 속도 (도/초)
    TurnSpeed = 180             -- 초당 회전할 수 있는 최대 각도
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
    -- 몽타주 재생 중이면 공격 중으로 판단
    local bMontagePlayling = IsMontagePlayling(Obj)
    local result = not bMontagePlayling
    Log("  [Cond] IsNotAttacking: " .. tostring(result) .. " (montage=" .. tostring(bMontagePlayling) .. ")")
    return result
end

local function IsAttacking(c)
    -- 몽타주 재생 중이면 공격 중으로 판단
    local bMontagePlayling = IsMontagePlayling(Obj)
    Log("  [Cond] IsAttacking: " .. tostring(bMontagePlayling))
    return bMontagePlayling
end

local function IsInStrafeRange(c)
    if not c.target or not c.self_actor then
        Log("  [Cond] IsInStrafeRange: false (no target)")
        return false
    end
    local dist = VecDistance(Obj.Location, c.target.Location)
    local result = dist >= Config.StrafeMinRange and dist <= Config.StrafeMaxRange
    Log("  [Cond] IsInStrafeRange: " .. tostring(result) .. " (dist=" .. string.format("%.2f", dist) .. "m, range=" .. Config.StrafeMinRange .. "-" .. Config.StrafeMaxRange .. "m)")
    return result
end

local function CanAttack(c)
    -- 몽타주 재생 중이면 불가
    local bMontagePlayling = IsMontagePlayling(Obj)
    if bMontagePlayling then
        Log("  [Cond] CanAttack: false (montage playing)")
        return false
    end
    -- 후퇴 중이면 불가
    if c.is_retreating then
        Log("  [Cond] CanAttack: false (retreating)")
        return false
    end
    -- 쿨다운 체크
    local timeSinceLastAttack = c.time - c.last_attack_time
    if timeSinceLastAttack < Config.AttackCooldown then
        Log("  [Cond] CanAttack: false (cooldown=" .. string.format("%.1f", Config.AttackCooldown - timeSinceLastAttack) .. "s)")
        return false
    end
    Log("  [Cond] CanAttack: true")
    return true
end

local function IsTooClose(c)
    if not c.target or not c.self_actor then
        return false
    end
    local dist = VecDistance(Obj.Location, c.target.Location)
    local result = dist < Config.RetreatRange
    Log("  [Cond] IsTooClose: " .. tostring(result) .. " (dist=" .. string.format("%.2f", dist) .. "m, range=" .. Config.RetreatRange .. "m)")
    return result
end

local function IsRetreating(c)
    local result = c.is_retreating and c.time < c.retreat_end_time
    Log("  [Cond] IsRetreating: " .. tostring(result))
    return result
end

local function ShouldRetreatAfterAttack(c)
    -- 공격 직후에 일정 확률로 후퇴
    -- was_attacking이 true였다가 false가 되는 순간 (공격 종료 직후)
    return math.random() < Config.RetreatChance
end

-- ============================================================================
-- Utility Functions (유틸리티 함수들)
-- ============================================================================

-- 각도 차이 계산 (-180 ~ 180 범위로 정규화)
local function NormalizeAngle(angle)
    while angle > 180 do angle = angle - 360 end
    while angle < -180 do angle = angle + 360 end
    return angle
end

-- 부드러운 회전 보간 (현재 Yaw에서 목표 Yaw로 delta만큼 회전)
local function SmoothRotateToTarget(currentYaw, targetYaw, maxDelta)
    local diff = NormalizeAngle(targetYaw - currentYaw)

    -- 회전량 제한
    if math.abs(diff) <= maxDelta then
        return targetYaw
    elseif diff > 0 then
        return currentYaw + maxDelta
    else
        return currentYaw - maxDelta
    end
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

    -- 몽타주 재생 중이면 이동 불가
    if IsMontagePlayling(Obj) then
        Log("  [Action] DoChase -> SKIP (montage playing)")
        return BT_SUCCESS
    end

    -- FVector를 Lua 테이블로 변환
    local myPosRaw = Obj.Location
    local targetPosRaw = c.target.Location
    local myPos = { X = myPosRaw.X, Y = myPosRaw.Y, Z = myPosRaw.Z }
    local targetPos = { X = targetPosRaw.X, Y = targetPosRaw.Y, Z = targetPosRaw.Z }

    local dir = VecDirection(myPos, targetPos)
    dir.Z = 0  -- 수평 이동만

    local dist = VecDistance(myPos, targetPos)

    -- 타겟을 바라보도록 부드러운 회전
    local targetYaw = CalcYaw(myPos, targetPos)
    local currentYaw = Obj.Rotation.Z
    local maxTurnDelta = Config.TurnSpeed * c.delta_time
    local newYaw = SmoothRotateToTarget(currentYaw, targetYaw, maxTurnDelta)
    Obj.Rotation = Vector(0, 0, newYaw)

    -- CharacterMovementComponent 사용
    local moveDir = Vector(dir.X, dir.Y, dir.Z)
    AddMovementInput(Obj, moveDir, 1.0)

    Log("  [Action] DoChase -> Moving (dist=" .. string.format("%.1f", dist) .. ", yaw=" .. string.format("%.1f", newYaw) .. " -> " .. string.format("%.1f", targetYaw) .. ")")
    return BT_SUCCESS
end

local function DoIdle(c)
    Log("  [Action] DoIdle")
    return BT_SUCCESS
end

local function DoStrafe(c)
    if not c.target then
        Log("  [Action] DoStrafe -> FAILURE (no target)")
        return BT_FAILURE
    end

    -- 몽타주 재생 중이면 이동 불가
    if IsMontagePlayling(Obj) then
        Log("  [Action] DoStrafe -> SKIP (montage playing)")
        return BT_SUCCESS
    end

    -- 일정 시간마다 방향 전환
    if c.time - c.strafe_last_change_time >= Config.StrafeChangeInterval then
        c.strafe_direction = c.strafe_direction * -1  -- 방향 반전
        c.strafe_last_change_time = c.time
        Log("    -> Strafe direction changed to: " .. (c.strafe_direction == 1 and "Right" or "Left"))
    end

    local myPosRaw = Obj.Location
    local targetPosRaw = c.target.Location
    local myPos = { X = myPosRaw.X, Y = myPosRaw.Y, Z = myPosRaw.Z }
    local targetPos = { X = targetPosRaw.X, Y = targetPosRaw.Y, Z = targetPosRaw.Z }

    -- 타겟을 바라보도록 부드러운 회전
    local targetYaw = CalcYaw(myPos, targetPos)
    local currentYaw = Obj.Rotation.Z
    local maxTurnDelta = Config.TurnSpeed * c.delta_time
    local newYaw = SmoothRotateToTarget(currentYaw, targetYaw, maxTurnDelta)
    Obj.Rotation = Vector(0, 0, newYaw)

    -- 타겟 방향 벡터
    local toTarget = VecDirection(myPos, targetPos)
    toTarget.Z = 0

    -- 좌우 방향 벡터 (타겟 방향의 수직 벡터)
    local strafeDir = {
        X = -toTarget.Y * c.strafe_direction,
        Y = toTarget.X * c.strafe_direction,
        Z = 0
    }

    -- AddMovementInput 사용
    local moveDir = Vector(strafeDir.X, strafeDir.Y, strafeDir.Z)
    AddMovementInput(Obj, moveDir, 1.0)

    local dist = VecDistance(myPos, targetPos)
    Log("  [Action] DoStrafe -> Moving " .. (c.strafe_direction == 1 and "Right" or "Left") .. " (dist=" .. string.format("%.2f", dist) .. "m)")
    return BT_SUCCESS
end

-- ============================================================================
-- Attack Actions (공격 행동들)
-- ============================================================================

local function StartAttack(c, patternName)
    c.last_attack_time = c.time
    Log("    -> StartAttack: " .. patternName)
end

-- 공격 패턴 정보 (히트박스는 애님 노티파이에서 처리)
local AttackPatterns = {
    [0] = { name = "LightCombo" },
    [1] = { name = "HeavySlam" },
    [2] = { name = "ChargeAttack" },
    [3] = { name = "SpinAttack" }
}

-- 랜덤 공격 패턴 선택 및 실행
local function DoAttack(c)
    Log("  [Action] DoAttack")

    -- 몽타주 재생 중이면 공격 불가
    if IsMontagePlayling(Obj) then
        Log("    -> FAILURE (montage playing)")
        return BT_FAILURE
    end

    -- 타겟 바라보기
    if c.target then
        local myPos = Obj.Location
        local targetPos = c.target.Location
        local yaw = CalcYaw(myPos, targetPos)
        Obj.Rotation = Vector(0, 0, yaw)
    end

    -- 페이즈에 따라 패턴 선택
    local pattern
    local maxPattern
    if c.phase >= 2 then
        -- Phase 2: 모든 패턴 사용 (0~3)
        maxPattern = 3
        pattern = math.random(0, maxPattern)
    else
        -- Phase 1: 기본 패턴만 (0~1)
        maxPattern = 1
        pattern = math.random(0, maxPattern)
    end

    -- 같은 패턴 연속 방지 (페이즈 범위 내에서)
    if pattern == c.last_pattern then
        pattern = (pattern + 1) % (maxPattern + 1)
    end
    c.last_pattern = pattern

    local atk = AttackPatterns[pattern]
    if not atk then
        Log("    -> FAILURE (invalid pattern)")
        return BT_FAILURE
    end

    -- 몽타주 재생
    local success = PlayMontage(Obj, atk.name)
    if not success then
        Log("    -> FAILURE (montage not found: " .. atk.name .. ")")
        return BT_FAILURE
    end

    -- ChargeAttack 특수 처리: 전방 돌진
    if pattern == 2 and c.target then
        local dir = VecDirection(Obj.Location, c.target.Location)
        dir.Z = 0
        Obj.Location = Vector(
            Obj.Location.X + dir.X * 500,
            Obj.Location.Y + dir.Y * 500,
            Obj.Location.Z
        )
        Log("    -> ChargeAttack: Charged forward 500 units")
    end

    -- 히트박스는 애님 노티파이에서 처리됨
    StartAttack(c, atk.name)
    return BT_SUCCESS
end

local function DoRetreat(c)
    Log("  [Action] DoRetreat")
    if not c.target then
        Log("    -> FAILURE (no target)")
        return BT_FAILURE
    end

    -- 몽타주 재생 중이면 이동 불가
    if IsMontagePlayling(Obj) then
        Log("    -> SKIP (montage playing)")
        return BT_SUCCESS
    end

    -- 후퇴 시작
    if not c.is_retreating then
        c.is_retreating = true
        c.retreat_end_time = c.time + Config.RetreatDuration
        Log("    -> Retreat started (duration=" .. Config.RetreatDuration .. "s)")
    end

    -- 후퇴 종료 체크
    if c.time >= c.retreat_end_time then
        c.is_retreating = false
        Log("    -> Retreat ended")
        return BT_SUCCESS
    end

    local myPosRaw = Obj.Location
    local targetPosRaw = c.target.Location
    local myPos = { X = myPosRaw.X, Y = myPosRaw.Y, Z = myPosRaw.Z }
    local targetPos = { X = targetPosRaw.X, Y = targetPosRaw.Y, Z = targetPosRaw.Z }

    -- 타겟을 바라보면서 후퇴 (부드러운 회전)
    local targetYaw = CalcYaw(myPos, targetPos)
    local currentYaw = Obj.Rotation.Z
    local maxTurnDelta = Config.TurnSpeed * c.delta_time
    local newYaw = SmoothRotateToTarget(currentYaw, targetYaw, maxTurnDelta)
    Obj.Rotation = Vector(0, 0, newYaw)

    -- 뒤로 이동 (AddMovementInput 사용)
    local dir = VecDirection(targetPos, myPos)  -- 타겟 반대 방향
    dir.Z = 0
    local moveDir = Vector(dir.X, dir.Y, dir.Z)
    AddMovementInput(Obj, moveDir, 1.0)

    Log("    -> Retreating...")
    return BT_RUNNING  -- 후퇴 중에는 RUNNING 반환
end

-- 후퇴 시작 함수 (공격 후 호출)
local function StartRetreat(c)
    c.is_retreating = true
    c.retreat_end_time = c.time + Config.RetreatDuration
    Log("  [Action] StartRetreat -> duration=" .. Config.RetreatDuration .. "s")
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
    -- 몽타주 기반 공격 종료 체크
    local bMontagePlayling = IsMontagePlayling(Obj)

    -- 이전에 공격 중이었는데 몽타주가 끝났으면 공격 종료 처리
    if c.was_attacking and not bMontagePlayling then
        c.was_attacking = false
        Log("Attack ended (montage finished)")

        -- 히트박스는 애님 노티파이의 Duration으로 자동 비활성화됨

        -- 확률적으로 후퇴 시작
        if ShouldRetreatAfterAttack(c) then
            StartRetreat(c)
        end
    end

    -- 현재 몽타주 재생 상태 저장
    c.was_attacking = bMontagePlayling

    -- 후퇴 상태 업데이트
    if c.is_retreating and c.time >= c.retreat_end_time then
        c.is_retreating = false
        Log("Retreat ended (timeout)")
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

    -- 2. 공격 중이면 대기
    Sequence({
        Condition(function(c)
            Log("[Branch 2] Attacking?")
            return IsAttacking(c)
        end),
        Action(DoIdle)
    }),

    -- 3. 후퇴 중이면 후퇴 계속
    Sequence({
        Condition(function(c)
            Log("[Branch 3] Retreating?")
            return IsRetreating(c)
        end),
        Action(DoRetreat)
    }),

    -- 4. 너무 가까우면 후퇴 (공격 쿨다운 중일 때)
    Sequence({
        Condition(function(c)
            Log("[Branch 4] TooClose?")
            return HasTarget(c) and IsTooClose(c) and IsNotAttacking(c) and not CanAttack(c)
        end),
        Action(DoRetreat)
    }),

    -- 5. 공격 범위 내 + 쿨다운 완료 → 공격
    Sequence({
        Condition(function(c)
            Log("[Branch 5] Attack?")
            return HasTarget(c) and IsTargetInAttackRange(c) and CanAttack(c)
        end),
        Action(DoAttack)
    }),

    -- 6. 일정 거리(4~8m)일 때 좌우 이동
    Sequence({
        Condition(function(c)
            Log("[Branch 6] Strafe?")
            return HasTarget(c) and IsInStrafeRange(c) and IsNotAttacking(c)
        end),
        Action(DoStrafe)
    }),

    -- 7. 추적 (공격 중이 아닐 때만)
    Sequence({
        Condition(function(c)
            Log("[Branch 7] Chase?")
            return HasTarget(c) and IsNotAttacking(c)
        end),
        Action(DoChase)
    }),

    -- 8. 기본 - 대기
    Action(function(c)
        Log("[Branch 8] Idle (fallback)")
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
    ctx.was_attacking = false

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
    ctx.delta_time = Delta

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
