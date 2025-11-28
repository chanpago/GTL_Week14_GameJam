#include "pch.h"
#include "DOFBlurPass.h"
#include "../SceneView.h"
#include "../../RHI/SwapGuard.h"
#include "../../RHI/ConstantBufferType.h"
#include "../../RHI/D3D11RHI.h"

void FDOFBlurPass::Execute(const FPostProcessModifier& M, FSceneView* View, D3D11RHI* RHIDevice)
{
    if (!IsApplicable(M)) return;

    // 셰이더 로드
    UShader* FullScreenVS = UResourceManager::GetInstance().Load<UShader>("Shaders/Utility/FullScreenTriangle_VS.hlsl");
    UShader* BlurPS = UResourceManager::GetInstance().Load<UShader>("Shaders/PostProcess/DOF_Blur_PS.hlsl");

    if (!FullScreenVS || !BlurPS || !FullScreenVS->GetVertexShader() || !BlurPS->GetPixelShader())
    {
        UE_LOG("DOFBlur: 셰이더 로드 실패!\n");
        return;
    }

    RHIDevice->PrepareShader(FullScreenVS, BlurPS);

    // Sampler
    ID3D11SamplerState* LinearClamp = RHIDevice->GetSamplerState(RHI_Sampler_Index::LinearClamp);
    if (!LinearClamp)
    {
        UE_LOG("DOFBlur: Sampler 없음!\n");
        return;
    }
    RHIDevice->GetDeviceContext()->PSSetSamplers(0, 1, &LinearClamp);

    // Depth/Blend State
    RHIDevice->OMSetDepthStencilState(EComparisonFunc::Always);
    RHIDevice->OMSetBlendState(false);

    // Viewport Constants (b10)
    FViewportConstants ViewportCB;
    ViewportCB.ViewportRect = FVector4(
        View->ViewRect.MinX,
        View->ViewRect.MinY,
        View->ViewRect.Width(),
        View->ViewRect.Height()
    );
    ViewportCB.ScreenSize = FVector4(
        RHIDevice->GetViewportWidth(),
        RHIDevice->GetViewportHeight(),
        1.0f / RHIDevice->GetViewportWidth(),
        1.0f / RHIDevice->GetViewportHeight()
    );
    RHIDevice->SetAndUpdateConstantBuffer(ViewportCB);

    // Blur Radius (MaxBlurSize from DOF params)
    float BlurRadius = FMath::Max(M.Payload.Params1.Z, M.Payload.Params1.W);  // max(MaxNearBlur, MaxFarBlur)

    ID3D11ShaderResourceView* NullSRV = nullptr;

    // ===========================================
    // 1. Far Field - Horizontal Blur
    // ===========================================
    {
        // SRV 언바인드
        RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &NullSRV);

        // RTV 설정: DOFTextures[2] (Temp)
        ID3D11RenderTargetView* tempRTV = RHIDevice->GetDOFRTV(2);
        RHIDevice->OMSetCustomRenderTargets(1, &tempRTV, nullptr);

        // Clear
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        RHIDevice->GetDeviceContext()->ClearRenderTargetView(tempRTV, clearColor);

        // SRV 바인딩: DOFTextures[0] (Far Field)
        ID3D11ShaderResourceView* farSRV = RHIDevice->GetSRV(RHI_SRV_Index::DOFFar);
        RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &farSRV);

        // 상수 버퍼: Horizontal
        FDOFBlurBufferType blurCB;
        blurCB.BlurDirection = FVector2(1.0f, 0.0f);
        blurCB.BlurRadius = BlurRadius;
        blurCB._Pad0 = 0.0f;
        RHIDevice->SetAndUpdateConstantBuffer(blurCB);

        // Draw
        RHIDevice->DrawFullScreenQuad();

        // SRV 언바인드
        RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &NullSRV);
    }

    // ===========================================
    // 2. Far Field - Vertical Blur
    // ===========================================
    {
        // RTV 설정: DOFTextures[0] (Far Field - 최종)
        ID3D11RenderTargetView* farRTV = RHIDevice->GetDOFRTV(0);
        RHIDevice->OMSetCustomRenderTargets(1, &farRTV, nullptr);

        // Clear
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        RHIDevice->GetDeviceContext()->ClearRenderTargetView(farRTV, clearColor);

        // SRV 바인딩: DOFTextures[2] (Temp)
        ID3D11ShaderResourceView* tempSRV = RHIDevice->GetSRV(RHI_SRV_Index::DOFTempH);
        RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &tempSRV);

        // 상수 버퍼: Vertical
        FDOFBlurBufferType blurCB;
        blurCB.BlurDirection = FVector2(0.0f, 1.0f);
        blurCB.BlurRadius = BlurRadius;
        blurCB._Pad0 = 0.0f;
        RHIDevice->SetAndUpdateConstantBuffer(blurCB);

        // Draw
        RHIDevice->DrawFullScreenQuad();

        // SRV 언바인드
        RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &NullSRV);
    }

    // ===========================================
    // 3. Near Field - Horizontal Blur
    // ===========================================
    {
        // RTV 설정: DOFTextures[2] (Temp)
        ID3D11RenderTargetView* tempRTV = RHIDevice->GetDOFRTV(2);
        RHIDevice->OMSetCustomRenderTargets(1, &tempRTV, nullptr);

        // Clear
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        RHIDevice->GetDeviceContext()->ClearRenderTargetView(tempRTV, clearColor);

        // SRV 바인딩: DOFTextures[1] (Near Field)
        ID3D11ShaderResourceView* nearSRV = RHIDevice->GetSRV(RHI_SRV_Index::DOFNear);
        RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &nearSRV);

        // 상수 버퍼: Horizontal
        FDOFBlurBufferType blurCB;
        blurCB.BlurDirection = FVector2(1.0f, 0.0f);
        blurCB.BlurRadius = BlurRadius;
        blurCB._Pad0 = 0.0f;
        RHIDevice->SetAndUpdateConstantBuffer(blurCB);

        // Draw
        RHIDevice->DrawFullScreenQuad();

        // SRV 언바인드
        RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &NullSRV);
    }

    // ===========================================
    // 4. Near Field - Vertical Blur
    // ===========================================
    {
        // RTV 설정: DOFTextures[1] (Near Field - 최종)
        ID3D11RenderTargetView* nearRTV = RHIDevice->GetDOFRTV(1);
        RHIDevice->OMSetCustomRenderTargets(1, &nearRTV, nullptr);

        // Clear
        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        RHIDevice->GetDeviceContext()->ClearRenderTargetView(nearRTV, clearColor);

        // SRV 바인딩: DOFTextures[2] (Temp)
        ID3D11ShaderResourceView* tempSRV = RHIDevice->GetSRV(RHI_SRV_Index::DOFTempH);
        RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &tempSRV);

        // 상수 버퍼: Vertical
        FDOFBlurBufferType blurCB;
        blurCB.BlurDirection = FVector2(0.0f, 1.0f);
        blurCB.BlurRadius = BlurRadius;
        blurCB._Pad0 = 0.0f;
        RHIDevice->SetAndUpdateConstantBuffer(blurCB);

        // Draw
        RHIDevice->DrawFullScreenQuad();

        // SRV 언바인드
        RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &NullSRV);
    }

    // RTV 복원
    RHIDevice->OMSetRenderTargets(ERTVMode::SceneColorTargetWithoutDepth);
}
