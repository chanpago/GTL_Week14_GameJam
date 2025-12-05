-- ============================================================================
-- BehaviorTree.lua - Lua용 Behavior Tree 프레임워크
-- ============================================================================

BT_SUCCESS = "SUCCESS"
BT_FAILURE = "FAILURE"
BT_RUNNING = "RUNNING"

-- ============================================================================
-- Composite Nodes
-- ============================================================================

-- Selector: 자식 중 하나가 SUCCESS 할 때까지 시도
function Selector(children)
    return function(context)
        for _, child in ipairs(children) do
            local result = child(context)
            if result ~= BT_FAILURE then
                return result  -- SUCCESS or RUNNING
            end
        end
        return BT_FAILURE
    end
end

-- Sequence: 모든 자식이 SUCCESS 해야 성공
function Sequence(children)
    return function(context)
        for _, child in ipairs(children) do
            local result = child(context)
            if result ~= BT_SUCCESS then
                return result  -- FAILURE or RUNNING
            end
        end
        return BT_SUCCESS
    end
end

-- RandomSelector: 자식 중 랜덤으로 하나 선택
function RandomSelector(children)
    return function(context)
        local shuffled = {}
        for i, v in ipairs(children) do
            shuffled[i] = v
        end
        -- Fisher-Yates shuffle
        for i = #shuffled, 2, -1 do
            local j = math.random(i)
            shuffled[i], shuffled[j] = shuffled[j], shuffled[i]
        end
        for _, child in ipairs(shuffled) do
            local result = child(context)
            if result ~= BT_FAILURE then
                return result
            end
        end
        return BT_FAILURE
    end
end

-- ============================================================================
-- Decorator Nodes
-- ============================================================================

-- Inverter: 결과 반전
function Inverter(child)
    return function(context)
        local result = child(context)
        if result == BT_SUCCESS then return BT_FAILURE
        elseif result == BT_FAILURE then return BT_SUCCESS
        else return result end
    end
end

-- Succeeder: 항상 SUCCESS 반환
function Succeeder(child)
    return function(context)
        child(context)
        return BT_SUCCESS
    end
end

-- Repeater: N번 반복
function Repeater(times, child)
    return function(context)
        for i = 1, times do
            local result = child(context)
            if result == BT_FAILURE then
                return BT_FAILURE
            end
        end
        return BT_SUCCESS
    end
end

-- Cooldown: 쿨다운 (마지막 성공 후 일정 시간 동안 FAILURE)
function Cooldown(duration, child)
    local last_success_time = -9999
    return function(context)
        local now = context.time or 0
        if now - last_success_time < duration then
            return BT_FAILURE
        end
        local result = child(context)
        if result == BT_SUCCESS then
            last_success_time = now
        end
        return result
    end
end

-- Condition: 조건 체크
function Condition(predicate)
    return function(context)
        if predicate(context) then
            return BT_SUCCESS
        else
            return BT_FAILURE
        end
    end
end

-- ============================================================================
-- Leaf Nodes
-- ============================================================================

-- Action: 실제 행동 수행
function Action(fn)
    return function(context)
        return fn(context) or BT_SUCCESS
    end
end

-- Wait: 일정 시간 대기 (RUNNING 반환)
function Wait(duration)
    local start_time = nil
    return function(context)
        local now = context.time or 0
        if start_time == nil then
            start_time = now
        end
        if now - start_time >= duration then
            start_time = nil
            return BT_SUCCESS
        end
        return BT_RUNNING
    end
end

-- ============================================================================
-- Utility Functions
-- ============================================================================

-- 벡터 생성 (Vector 전역함수가 없을 경우 대비)
function MakeVector(x, y, z)
    if Vector then
        return Vector(x, y, z)
    else
        return { X = x, Y = y, Z = z }
    end
end

-- 벡터 길이
function VecLength(v)
    return math.sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z)
end

-- 벡터 정규화
function VecNormalize(v)
    local len = VecLength(v)
    if len > 0.0001 then
        return MakeVector(v.X / len, v.Y / len, v.Z / len)
    end
    return MakeVector(0, 0, 0)
end

-- 두 위치 사이의 거리
function VecDistance(a, b)
    local dx = b.X - a.X
    local dy = b.Y - a.Y
    local dz = b.Z - a.Z
    return math.sqrt(dx * dx + dy * dy + dz * dz)
end

-- 방향 벡터 계산 (a에서 b로)
function VecDirection(from, to)
    local dir = MakeVector(to.X - from.X, to.Y - from.Y, to.Z - from.Z)
    return VecNormalize(dir)
end

-- 방향 벡터에서 Yaw 각도 계산
function CalcYaw(myPos, targetPos)
    local dir = VecDirection(myPos, targetPos)

    local yaw = 0
    if dir.X ~= 0 then
        yaw = math.atan(dir.Y / dir.X) * (180.0 / math.pi)
        if dir.X < 0 then
            yaw = yaw + 180
        end
    else
        if dir.Y > 0 then
            yaw = 90
        elseif dir.Y < 0 then
            yaw = -90
        end
    end

    return yaw
end

-- 방향 벡터로부터 Yaw 각도 계산
function GetYawFromDirection(dir)
    local yaw = 0
    if dir.X ~= 0 then
        yaw = math.atan(dir.Y / dir.X) * (180.0 / math.pi)
        if dir.X < 0 then
            yaw = yaw + 180
        end
    else
        if dir.Y > 0 then
            yaw = 90
        elseif dir.Y < 0 then
            yaw = -90
        end
    end
    return yaw
end

print("[BehaviorTree] Framework loaded")
