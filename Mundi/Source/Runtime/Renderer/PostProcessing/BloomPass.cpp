#include "pch.h"
#include "BloomPass.h"
#include "../SceneView.h"
#include "../../RHI/SwapGuard.h"
#include "../../RHI/ConstantBufferType.h"
#include "../../RHI/D3D11RHI.h"

void FBloomPass::Execute(const FPostProcessModifier& M, FSceneView* View, D3D11RHI* RHIDevice)
{
    if (!IsApplicable(M) || !RHIDevice)
    {
        return;
    }

    UShader* FullScreenVS = UResourceManager::GetInstance().Load<UShader>("Shaders/Utility/FullScreenTriangle_VS.hlsl");
    UShader* BloomPS = UResourceManager::GetInstance().Load<UShader>("Shaders/PostProcess/Bloom_PS.hlsl");
    if (!FullScreenVS || !BloomPS || !FullScreenVS->GetVertexShader() || !BloomPS->GetPixelShader())
    {
        UE_LOG("BloomPass: shader load failed!\n");
        return;
    }

    ID3D11DeviceContext* Context = RHIDevice->GetDeviceContext();
    if (!Context)
    {
        return;
    }

    const UINT fullWidth = std::max(1u, RHIDevice->GetSwapChainWidth());
    const UINT fullHeight = std::max(1u, RHIDevice->GetSwapChainHeight());
    const UINT bloomWidth = std::max(1u, fullWidth / 2);
    const UINT bloomHeight = std::max(1u, fullHeight / 2);

    ID3D11RenderTargetView* PrefilterRTV = RHIDevice->GetBloomRTV(0);
    ID3D11RenderTargetView* TempRTV = RHIDevice->GetBloomRTV(1);      // Ping Pong용
    ID3D11ShaderResourceView* PrefilterSRV = RHIDevice->GetBloomSRV(0);
    ID3D11ShaderResourceView* TempSRV = RHIDevice->GetBloomSRV(1);
    if (!PrefilterRTV || !TempRTV || !PrefilterSRV || !TempSRV)
    {
        UE_LOG("BloomPass: bloom render targets not initialized!");
        return;
    }

    ID3D11SamplerState* LinearClamp = RHIDevice->GetSamplerState(RHI_Sampler_Index::LinearClamp);
    if (!LinearClamp)
    {
        UE_LOG("BloomPass: missing sampler");
        return;
    }

    D3D11_VIEWPORT originalViewport;
    UINT viewportCount = 1;
    Context->RSGetViewports(&viewportCount, &originalViewport);

    D3D11_VIEWPORT bloomViewport = {};
    bloomViewport.TopLeftX = 0.0f;
    bloomViewport.TopLeftY = 0.0f;
    bloomViewport.Width = static_cast<float>(bloomWidth);
    bloomViewport.Height = static_cast<float>(bloomHeight);
    bloomViewport.MinDepth = 0.0f;
    bloomViewport.MaxDepth = 1.0f;
    Context->RSSetViewports(1, &bloomViewport);

    RHIDevice->PrepareShader(FullScreenVS, BloomPS);
    Context->PSSetSamplers(0, 1, &LinearClamp);

    RHIDevice->OMSetDepthStencilState(EComparisonFunc::Always);
    RHIDevice->OMSetBlendState(false);

    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    const float threshold = M.Payload.Params0.X;
    const float softKnee = std::max(M.Payload.Params0.Y, 0.0f);
    const float bloomIntensity = M.Payload.Params0.Z * M.Weight;
    const float blurRadius = std::max(M.Payload.Params1.X, 0.5f);

    FBloomSettingsBufferType BloomCB;
    BloomCB.Params0 = FVector4(threshold, softKnee, bloomIntensity, 0.0f);
    BloomCB.Params1 = FVector4(1.0f / static_cast<float>(bloomWidth), 1.0f / static_cast<float>(bloomHeight), blurRadius, 0.0f); // For Texel 범위
    BloomCB.Params2 = FVector4(0.0f, 0.0f, 0.0f, 0.0f);

    ID3D11ShaderResourceView* NullSRVs[2] = { nullptr, nullptr };

    // Prefilter (bright pass)
    RHIDevice->OMSetCustomRenderTargets(1, &PrefilterRTV, nullptr);
    Context->ClearRenderTargetView(PrefilterRTV, clearColor);

    ID3D11ShaderResourceView* SceneSRV = RHIDevice->GetSRV(RHI_SRV_Index::SceneColorSource);
    if (!SceneSRV)
    {
        UE_LOG("BloomPass: SceneColorSource SRV missing\n");
        Context->RSSetViewports(1, &originalViewport);
        return;
    }
    Context->PSSetShaderResources(0, 1, &SceneSRV);
    RHIDevice->SetAndUpdateConstantBuffer(BloomCB);
    RHIDevice->DrawFullScreenQuad();
    Context->PSSetShaderResources(0, 1, &NullSRVs[0]);

    // Twopass Gaussian Blur : 좌우로 먼저 Blur 실행, 해당 결과를 이용해 상하로 Blur 실행
    // Horizontal blur
    RHIDevice->OMSetCustomRenderTargets(1, &TempRTV, nullptr);
    Context->ClearRenderTargetView(TempRTV, clearColor);
    BloomCB.Params0.W = 1.0f;
    BloomCB.Params2 = FVector4(1.0f, 0.0f, 0.0f, 0.0f);
    RHIDevice->SetAndUpdateConstantBuffer(BloomCB);
    Context->PSSetShaderResources(0, 1, &PrefilterSRV);
    RHIDevice->DrawFullScreenQuad();
    Context->PSSetShaderResources(0, 1, &NullSRVs[0]);

    // Vertical blur back into Prefilter buffer (final bloom texture)
    RHIDevice->OMSetCustomRenderTargets(1, &PrefilterRTV, nullptr);
    Context->ClearRenderTargetView(PrefilterRTV, clearColor);
    BloomCB.Params2 = FVector4(0.0f, 1.0f, 0.0f, 0.0f);
    RHIDevice->SetAndUpdateConstantBuffer(BloomCB);
    Context->PSSetShaderResources(0, 1, &TempSRV);
    RHIDevice->DrawFullScreenQuad();
    Context->PSSetShaderResources(0, 1, &NullSRVs[0]);

    // Composite with the original scene (restore viewport) : 원래 Viewport에 합성
    Context->RSSetViewports(1, &originalViewport);

    FSwapGuard SwapGuard(RHIDevice, 0, 2);
    RHIDevice->OMSetRenderTargets(ERTVMode::SceneColorTargetWithoutDepth);

    BloomCB.Params0.W = 2.0f;
    BloomCB.Params1 = FVector4(1.0f / static_cast<float>(fullWidth), 1.0f / static_cast<float>(fullHeight), blurRadius, 0.0f);
    BloomCB.Params2 = FVector4(0.0f, 0.0f, 0.0f, 0.0f);
    RHIDevice->SetAndUpdateConstantBuffer(BloomCB);

    ID3D11ShaderResourceView* CompositeSRVs[2] = { RHIDevice->GetSRV(RHI_SRV_Index::SceneColorSource), PrefilterSRV };
    Context->PSSetShaderResources(0, 2, CompositeSRVs);
    RHIDevice->DrawFullScreenQuad();
    Context->PSSetShaderResources(0, 2, NullSRVs);

    SwapGuard.Commit();
}