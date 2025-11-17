#include "pch.h"
#include "GPUProfile.h"

void FGPUProfiler::Initialize(D3D11RHI* InRHI)
{
    RHIDevice = InRHI;
    Device = RHIDevice->GetDevice();
    DeviceContext = RHIDevice->GetDeviceContext();

    // 초기 쿼리 풀을 미리 생성합니다 (예: 100개)
    const int32 InitialPoolSize = 20;
    for (int32 i = 0; i < InitialPoolSize; ++i)
    {
        FreeQueryPool.Add(CreateNewQueryPair());
    }
}

void FGPUProfiler::Shutdown()
{
    // 풀에 있는 모든 쿼리 해제
    for (FQueryPair* Query : FreeQueryPool)
    {
        Query->DisjointQuery->Release();
        Query->BeginQuery->Release();
        Query->EndQuery->Release();
        delete Query;
    }
    FreeQueryPool.Empty();

    // 대기 중인 쿼리도 모두 해제 (정상 종료 시 비어있어야 함)
    for (FPendingQuery& Pending : PendingQueries)
    {
        Pending.Query->DisjointQuery->Release();
        Pending.Query->BeginQuery->Release();
        Pending.Query->EndQuery->Release();
        delete Pending.Query;
    }
    PendingQueries.Empty();
}

FQueryPair* FGPUProfiler::CreateNewQueryPair()
{
    FQueryPair* NewPair = new FQueryPair();
    D3D11_QUERY_DESC QueryDesc = {};

    QueryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    Device->CreateQuery(&QueryDesc, &NewPair->DisjointQuery);

    QueryDesc.Query = D3D11_QUERY_TIMESTAMP;
    Device->CreateQuery(&QueryDesc, &NewPair->BeginQuery);
    Device->CreateQuery(&QueryDesc, &NewPair->EndQuery);
    
    return NewPair;
}

FQueryPair* FGPUProfiler::AcquireQuery()
{
    FQueryPair* Query = nullptr;
    if (FreeQueryPool.Num() > 0)
    {
        Query = FreeQueryPool.Pop(); // Pop(false)는 배열 축소를 수행하지 않음
    }
    else
    {
        // 풀이 비어있으면 새로 생성 (경고 로그를 남기는 것이 좋음)
        Query = CreateNewQueryPair();
    }
    return Query;
}

void FGPUProfiler::SubmitQuery(const FString& InKey, FQueryPair* InQuery)
{
    PendingQueries.Add(FPendingQuery{InKey, InQuery});
}

void FGPUProfiler::BeginFrame()
{
    // 1. 이전 프레임의 결과 맵을 비웁니다.
    // (GPUTimeMap은 '가장 최근에 수집 완료된' 프레임의 데이터만 가짐)
    GPUTimeMap.Empty();

    // 2. 대기 중인 쿼리들을 검사합니다.
    // (배열을 뒤에서부터 순회해야 RemoveAt(i)가 안전함)
    for (int32 i = PendingQueries.Num() - 1; i >= 0; --i)
    {
        FPendingQuery& Pending = PendingQueries[i];

        D3D11_QUERY_DATA_TIMESTAMP_DISJOINT DisjointData = {};

        // D3D11_ASYNC_GETDATA_DONOTFLUSH 플래그로 절대 블로킹하지 않음
        const HRESULT DisjointResult = DeviceContext->GetData(
            Pending.Query->DisjointQuery, 
            &DisjointData, 
            sizeof(DisjointData), 
            D3D11_ASYNC_GETDATA_DONOTFLUSH
        );

        if (DisjointResult == S_OK)
        {
            // 쿼리 결과 수집 성공!
            double ElapsedTime = 0.0;

            if (!DisjointData.Disjoint)
            {
                // 타이밍이 유효함
                uint64 BeginTime = 0;
                uint64 EndTime = 0;

                // Disjoint가 S_OK이면 Begin/End도 S_OK를 반환해야 함 (D3D 스펙)
                // 만약의 경우를 대비해 여기서는 GetData(..., 0)을 사용하지만,
                // S_OK가 보장되므로 블로킹되지 않음.
                DeviceContext->GetData(Pending.Query->BeginQuery, &BeginTime, sizeof(BeginTime), 0);
                DeviceContext->GetData(Pending.Query->EndQuery, &EndTime, sizeof(EndTime), 0);

                const double Delta = static_cast<double>(EndTime - BeginTime);
                const double Frequency = static_cast<double>(DisjointData.Frequency);
                ElapsedTime = (Delta / Frequency) * 1000.0; // 밀리초(ms) 단위
            }
            // else: Disjoint == true. 타이밍이 유효하지 않음 (ElapsedTime = 0.0)

            // 결과 맵에 누적 (예: "Shadows"가 여러 번 호출될 경우 합산)
            GPUTimeMap[Pending.Key] += ElapsedTime;

            // 쿼리 객체를 풀에 반납
            FreeQueryPool.Add(Pending.Query);
            // 대기열에서 제거
            PendingQueries.RemoveAt(i);
        }
        else if (DisjointResult == S_FALSE)
        {
            // 데이터가 아직 준비되지 않음. 다음 프레임에 다시 시도.
            // 아무것도 하지 않고 대기열에 남겨둠.
        }
        else
        {
            // DXGI_ERROR_DEVICE_REMOVED 등 오류 발생
            // 쿼리를 버리고 풀에 반납
            FreeQueryPool.Add(Pending.Query);
            PendingQueries.RemoveAt(i);
        }
    }
}

double FGPUProfiler::GetStat(const FString& InKey) const
{
    // FindRef는 키가 없을 경우 0.0을 반환 (TMap의 기본값)
    return GPUTimeMap.FindRef(InKey);
}