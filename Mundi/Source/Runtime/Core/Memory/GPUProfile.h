#pragma once

// 헬퍼 매크로 (토큰 연결용)
#define PASTE_AUX(a, b) a##b
#define PASTE(a, b) PASTE_AUX(a, b)

#define GPU_PROFILER FGPUProfiler::GetInstance()
#define GPU_PROFILING_START GPU_PROFILER.BeginFrame();

/**
 * @brief 현재 스코프의 GPU 실행 시간을 측정합니다.
 * @param Key 측정할 작업의 이름 (FString 또는 const TCHAR*).
 */
#define GPU_TIME_PROFILE(Key) FScopedGPUTimer PASTE(GPUTimer_, __LINE__)(Key);
#define GET_GPU_STAT(Key) FGPUProfiler::GetInstance().GetStat(Key);

struct FQueryPair
{
    ID3D11Query* DisjointQuery = nullptr;
    ID3D11Query* BeginQuery = nullptr;
    ID3D11Query* EndQuery = nullptr;
};

/**
 * @brief 결과 수집 대기 중인 쿼리 정보
 */
struct FPendingQuery
{
    FString Key;
    FQueryPair* Query = nullptr;
};

/**
 * @brief 비동기 GPU 쿼리를 관리하는 중앙 프로파일러 (싱글톤)
 *
 * 쿼리 객체를 풀링하고, 쿼리 제출(N 프레임)과 결과 수집(N+k 프레임)을 분리하여
 * CPU-GPU 동기화로 인한 성능 저하를 방지합니다.
 */
class FGPUProfiler
{
public:
    /**
     * @brief 싱글톤 인스턴스에 접근합니다.
     */
    static FGPUProfiler& GetInstance()
    {
        static FGPUProfiler Instance;
        return Instance;
    }

    /**
     * @brief 프로파일러를 초기화합니다. (RHI 초기화 직후 호출)
     */
    void Initialize(D3D11RHI* InRHI);

    /**
     * @brief 프로파일러를 종료하고 모든 리소스를 해제합니다. (RHI 종료 직전 호출)
     */
    void Shutdown();

    /**
     * @brief 매 프레임 렌더링 시작 시 호출해야 합니다.
     *
     * GPUTimeMap을 초기화하고, 이전에 제출된 쿼리들의 결과를 비동기적으로 수집합니다.
     */
    void BeginFrame();

    /**
     * @brief 렌더링 통계 UI 등에서 특정 키의 측정 시간을 가져옵니다.
     * @note 이 값은 1~2 프레임 지연된 (가장 최근에 완료된) 결과입니다.
     */
    double GetStat(const FString& InKey) const;

    /**
     * @brief (FScopedGPUTimer 내부용) 쿼리 풀에서 사용 가능한 쿼리 객체를 가져옵니다.
     */
    FQueryPair* AcquireQuery();

    /**
     * @brief (FScopedGPUTimer 내부용) 사용 완료된 쿼리를 결과 대기열에 제출합니다.
     */
    void SubmitQuery(const FString& InKey, FQueryPair* InQuery);

    ID3D11DeviceContext* GetDeviceContext() const { return DeviceContext; }

private:
    FGPUProfiler() = default;
    ~FGPUProfiler() = default;

    FGPUProfiler(const FGPUProfiler&) = delete;
    FGPUProfiler& operator=(const FGPUProfiler&) = delete;

    // RHI 및 D3D 디바이스/컨텍스트
    D3D11RHI* RHIDevice = nullptr;
    ID3D11Device* Device = nullptr;
    ID3D11DeviceContext* DeviceContext = nullptr;

    /**
     * @brief 사용 가능한 FQueryPair 객체 풀
     */
    TArray<FQueryPair*> FreeQueryPool;

    /**
     * @brief N 프레임에 제출되어 N+k 프레임에서 결과를 기다리는 쿼리 목록
     */
    TArray<FPendingQuery> PendingQueries;

    /**
     * @brief 가장 최근에 수집된 프레임의 타이밍 결과
     */
    TMap<FString, double> GPUTimeMap;

    /**
     * @brief 새로운 FQueryPair 객체를 생성합니다.
     */
    FQueryPair* CreateNewQueryPair();
};


/**
 * @brief RAII 기반의 스코프 GPU 타이머
 *
 * 생성 시 타이머를 시작하고, 소멸 시 타이머를 종료하며 FGPUProfiler에 작업을 제출합니다.
 * 이 클래스 자체는 절대 CPU를 블로킹하지 않습니다.
 */
class FScopedGPUTimer
{
public:
    explicit FScopedGPUTimer(const FString& InKey)
        : Key(InKey)
    {        
        DeviceContext = FGPUProfiler::GetInstance().GetDeviceContext();     
        
        // 프로파일러에서 쿼리 객체 획득
        Query = FGPUProfiler::GetInstance().AcquireQuery();        

        // 타이머 시작
        DeviceContext->Begin(Query->DisjointQuery);
        DeviceContext->End(Query->BeginQuery);
    }

    ~FScopedGPUTimer()
    {
        // 타이머 종료
        DeviceContext->End(Query->EndQuery);
        DeviceContext->End(Query->DisjointQuery);

        // 프로파일러에 쿼리 제출 (결과 수집 대기)
        FGPUProfiler::GetInstance().SubmitQuery(Key, Query);
    }

    // 복사 및 이동 금지
    FScopedGPUTimer(const FScopedGPUTimer&) = delete;
    FScopedGPUTimer& operator=(const FScopedGPUTimer&) = delete;
    FScopedGPUTimer(FScopedGPUTimer&&) = delete;
    FScopedGPUTimer& operator=(FScopedGPUTimer&&) = delete;

private:
    FString Key;
    FQueryPair* Query = nullptr;
    ID3D11DeviceContext* DeviceContext = nullptr;
};