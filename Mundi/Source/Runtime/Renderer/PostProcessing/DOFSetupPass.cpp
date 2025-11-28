#include "pch.h"
#include "DOFSetupPass.h"
#include "../SceneView.h"
#include "../../RHI/SwapGuard.h"
#include "../../RHI/ConstantBufferType.h"
#include "../../RHI/D3D11RHI.h"

void FDOFSetupPass::Execute(const FPostProcessModifier& M, FSceneView* View, D3D11RHI* RHIDevice)
{
    if (!IsApplicable(M)) return;

    // 1) SRV 언바인드 (SceneColor + SceneDepth = 2개)
    ID3D11ShaderResourceView* NullSRVs[2] = { nullptr, nullptr };
    RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 2, NullSRVs);

    // 2) MRT 설정: Far Field (RTV[0]), Near Field (RTV[1])
    ID3D11RenderTargetView** DOFRTVs = RHIDevice->GetDOFRTVs();

    RHIDevice->OMSetCustomRenderTargets(2, DOFRTVs, nullptr);

    // 3) Clear RTV (검은색)
    float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    RHIDevice->GetDeviceContext()->ClearRenderTargetView(DOFRTVs[0], ClearColor);
    RHIDevice->GetDeviceContext()->ClearRenderTargetView(DOFRTVs[1], ClearColor);

    // 4) Depth State: Depth Test/Write OFF
    RHIDevice->OMSetDepthStencilState(EComparisonFunc::Always);
    RHIDevice->OMSetBlendState(false);

    // 5) 셰이더 로드
    UShader* FullScreenVS = UResourceManager::GetInstance().Load<UShader>("Shaders/Utility/FullScreenTriangle_VS.hlsl");
    UShader* SetupPS = UResourceManager::GetInstance().Load<UShader>("Shaders/PostProcess/DOF_Setup_PS.hlsl");

    if (!FullScreenVS || !SetupPS || !FullScreenVS->GetVertexShader() || !SetupPS->GetPixelShader())
    {
        UE_LOG("DOFSetup: 셰이더 로드 실패!\n");
        return;
    }

    RHIDevice->PrepareShader(FullScreenVS, SetupPS);

    // 6) SRV 바인딩 (t0: SceneColor, t1: SceneDepth)
    ID3D11ShaderResourceView* SceneSRV = RHIDevice->GetSRV(RHI_SRV_Index::SceneColorSource);
    ID3D11ShaderResourceView* DepthSRV = RHIDevice->GetSRV(RHI_SRV_Index::SceneDepth);
    ID3D11SamplerState* LinearClamp = RHIDevice->GetSamplerState(RHI_Sampler_Index::LinearClamp);
    ID3D11SamplerState* PointClamp = RHIDevice->GetSamplerState(RHI_Sampler_Index::PointClamp);

    if (!SceneSRV || !DepthSRV || !LinearClamp || !PointClamp)
    {
        UE_LOG("DOFSetup: SRV/Sampler 없음!\n");
        return;
    }

    ID3D11ShaderResourceView* InputSRVs[2] = { SceneSRV, DepthSRV };
    RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 2, InputSRVs);

    ID3D11SamplerState* Samplers[2] = { LinearClamp, PointClamp };
    RHIDevice->GetDeviceContext()->PSSetSamplers(0, 2, Samplers);

    // 7) 상수 버퍼 업데이트
    // b0: PostProcessCB
    PostProcessBufferType PostProcessCB;
    PostProcessCB.Near = View->NearClip;
    PostProcessCB.Far = View->FarClip;
    PostProcessCB.IsOrthographic = (View->ProjectionMode == ECameraProjectionMode::Orthographic) ? 1 : 0;
    PostProcessCB.Padding = 0.0f;
    RHIDevice->SetAndUpdateConstantBuffer(PostProcessCB);

    // b1: ViewProjBuffer (이미 설정되어 있을 수도 있지만 확실하게)
    ViewProjBufferType ViewProjCB;
    ViewProjCB.View = View->ViewMatrix;
    ViewProjCB.Proj = View->ProjectionMatrix;
    ViewProjCB.InvView = View->ViewMatrix.Inverse();
    ViewProjCB.InvProj = View->ProjectionMatrix.Inverse();
    RHIDevice->SetAndUpdateConstantBuffer(ViewProjCB);

    // b2: DOFSetupCB
    FDOFSetupBufferType DOFSetupCB;
    DOFSetupCB.FocalDistance = M.Payload.Params0.X;
    DOFSetupCB.Fstop = M.Payload.Params0.Y;
    DOFSetupCB.SensorWidth = M.Payload.Params0.Z;
    DOFSetupCB.FocalRegion = M.Payload.Params0.W;

    DOFSetupCB.NearTransitionRegion = M.Payload.Params1.X;
    DOFSetupCB.FarTransitionRegion = M.Payload.Params1.Y;
    DOFSetupCB.MaxNearBlurSize = M.Payload.Params1.Z;
    DOFSetupCB.MaxFarBlurSize = M.Payload.Params1.W;

    DOFSetupCB.NearClip = View->NearClip;
    DOFSetupCB.FarClip = View->FarClip;
    DOFSetupCB.IsOrthographic = (View->ProjectionMode == ECameraProjectionMode::Orthographic) ? 1 : 0;
    DOFSetupCB._Pad0 = 0.0f;

    RHIDevice->SetAndUpdateConstantBuffer(DOFSetupCB);
1
    // b10: ViewportConstants
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

    // 8) Draw
    RHIDevice->DrawFullScreenQuad();

    // 9) SRV 언바인드
    RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 2, NullSRVs);

    // 10) RTV를 SceneColor로 복원
    RHIDevice->OMSetRenderTargets(ERTVMode::SceneColorTargetWithoutDepth);
}
