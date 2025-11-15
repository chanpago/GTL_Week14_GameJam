#include "pch.h"
#include "SkeletalMesh.h"


#include "FbxLoader.h"
#include "WindowsBinReader.h"
#include "WindowsBinWriter.h"
#include "PathUtils.h"
#include <filesystem>

IMPLEMENT_CLASS(USkeletalMesh)

USkeletalMesh::USkeletalMesh()
{
}

USkeletalMesh::~USkeletalMesh()
{
    ReleaseResources();
}

void USkeletalMesh::Load(const FString& InFilePath, ID3D11Device* InDevice)
{
    if (Data)
    {
        ReleaseResources();
    }

    // FBXLoader가 캐싱을 내부적으로 처리합니다
    Data = UFbxLoader::GetInstance().LoadFbxMeshAsset(InFilePath);

    if (!Data || Data->Vertices.empty() || Data->Indices.empty())
    {
        UE_LOG("ERROR: Failed to load FBX mesh from '%s'", InFilePath.c_str());
        return;
    }

    // GPU 버퍼 생성
    CreateIndexBuffer(Data, InDevice);
    VertexCount = static_cast<uint32>(Data->Vertices.size());
    IndexCount = static_cast<uint32>(Data->Indices.size());
    CPUSkinnedVertexStride = sizeof(FVertexDynamic);
    GPUSkinnedVertexStride = sizeof(FSkinnedVertex);
}

void USkeletalMesh::ReleaseResources()
{
    if (IndexBuffer)
    {
        IndexBuffer->Release();
        IndexBuffer = nullptr;
    }

    if (Data)
    {
        delete Data;
        Data = nullptr;
    }
}

void USkeletalMesh::CreateCPUSkinnedVertexBuffer(ID3D11Buffer** InVertexBuffer)
{
    if (!Data) { return; }
    ID3D11Device* Device = GEngine.GetRHIDevice()->GetDevice();
    HRESULT hr = D3D11RHI::CreateVertexBuffer<FVertexDynamic>(Device, Data->Vertices, InVertexBuffer);
    assert(SUCCEEDED(hr));
}

void USkeletalMesh::CreateGPUSkinnedVertexBuffer(ID3D11Buffer** InVertexBuffer)
{
    if (!Data)
    {
        return;
    }
    ID3D11Device* Device = GEngine.GetRHIDevice()->GetDevice();
    HRESULT hr = D3D11RHI::CreateVertexBuffer<FSkinnedVertex>(Device, Data->Vertices, InVertexBuffer);
    assert(SUCCEEDED(hr));
}

void USkeletalMesh::UpdateVertexBuffer(const TArray<FNormalVertex>& SkinnedVertices, ID3D11Buffer* InVertexBuffer)
{
    if (!InVertexBuffer) { return; }

    GEngine.GetRHIDevice()->VertexBufferUpdate(InVertexBuffer, SkinnedVertices);
}

void USkeletalMesh::CreateStructuredBuffer(ID3D11Buffer** InStructuredBuffer, ID3D11ShaderResourceView** InShaderResourceView, UINT ElementCount)
{
    if (!InStructuredBuffer || !InShaderResourceView || !Data)
    {
        return;
    }

    D3D11RHI *RHI = GEngine.GetRHIDevice();
    HRESULT hr = RHI->CreateStructuredBuffer(sizeof(FMatrix), ElementCount, nullptr, InStructuredBuffer);
    if (FAILED(hr))
    {
        UE_LOG("[USkeletalMesh/CreateStructuredBuffer] Structured buffer ceation fail");
        return;
    }

    hr = RHI->CreateStructuredBufferSRV(*InStructuredBuffer, InShaderResourceView);
    if (FAILED(hr))
    {
        UE_LOG("[USkeletalMesh/CreateStructuredBuffer] Structured bufferSRV ceation fail");
        return;
    }    
}

void USkeletalMesh::CreateIndexBuffer(FSkeletalMeshData* InSkeletalMesh, ID3D11Device* InDevice)
{
    HRESULT hr = D3D11RHI::CreateIndexBuffer(InDevice, InSkeletalMesh, &IndexBuffer);
    assert(SUCCEEDED(hr));
}
