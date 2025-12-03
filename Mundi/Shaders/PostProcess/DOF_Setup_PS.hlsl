#include "DOF_Common.hlsli"

// Input Textures
Texture2D    g_SceneColorTex : register(t0);
Texture2D    g_SceneDepthTex : register(t1);

SamplerState g_LinearClampSample : register(s0);
SamplerState g_PointClampSample  : register(s1);

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

// Constant Buffers
cbuffer PostProcessCB : register(b0)
{
    float Near;
    float Far;
    int IsOrthographic;
    float Padding0;
}

cbuffer ViewProjBuffer : register(b1)
{
    row_major float4x4 ViewMatrix;
    row_major float4x4 ProjectionMatrix;
    row_major float4x4 InverseViewMatrix;
    row_major float4x4 InverseProjectionMatrix;
}

cbuffer DOFSetupCB : register(b2)
{
    float FocalDistance;           // m (meters)
    float FocalRegion;             // m
    float NearTransitionRegion;    // m
    float FarTransitionRegion;     // m

    float MaxNearBlurSize;         // pixels
    float MaxFarBlurSize;          // pixels
    float NearClip;
    float FarClip;

    int IsOrthographic2;
    float3 _Pad0;
}

cbuffer ViewportConstants : register(b10)
{
    float4 ViewportRect;
    float4 ScreenSize;
}

// MRT 출력 (Near/Far 분리)
struct PS_OUTPUT
{
    float4 Near : SV_Target0;  // Near 전용 (RGB + |CoC|, Focus/Far는 투명)
    float4 Far  : SV_Target1;  // Far 전용 (RGB + CoC, Focus/Near는 투명)
};

// 단일 픽셀 CoC 계산 헬퍼
float CalcCoCAtUV(float2 uv)
{
    float rawDepth = g_SceneDepthTex.Sample(g_PointClampSample, uv).r;
    float linearDepth = LinearizeDepth(rawDepth, NearClip, FarClip, IsOrthographic2);
    return CalculateCoCWithSkyCheck(
        linearDepth,
        FocalDistance,
        FocalRegion,
        NearTransitionRegion,
        FarTransitionRegion,
        MaxNearBlurSize,
        MaxFarBlurSize,
        FarClip
    );
}

PS_OUTPUT mainPS(PS_INPUT input)
{
    PS_OUTPUT output;

    // 1. Bilinear 다운샘플 (컬러)
    float4 sceneColor = g_SceneColorTex.Sample(g_LinearClampSample, input.texCoord);

    // 2. CoC 4x4 샘플링
    float2 texelSize = ScreenSize.zw;

    float nearCoCSum = 0.0;
    float farCoCSum = 0.0;

    [unroll]
    for (int y = 0; y < 4; y++)
    {
        [unroll]
        for (int x = 0; x < 4; x++)
        {
            float2 offset = float2(x - 1.5, y - 1.5) * texelSize;
            float coc = CalcCoCAtUV(input.texCoord + offset);

            nearCoCSum += max(-coc, 0);
            farCoCSum += max(coc, 0);
        }
    }

    // Near/Far 둘 다 평균
    float nearCoC = nearCoCSum / 16.0;
    float farCoC = farCoCSum / 16.0;

    // 3. Near/Far 분리 출력
    output.Near = float4(sceneColor.rgb, nearCoC);
    output.Far = float4(sceneColor.rgb, farCoC);

    return output;
}
