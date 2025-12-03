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

    // 1. Bilinear 다운샘플
    float4 sceneColor = g_SceneColorTex.Sample(g_LinearClampSample, input.texCoord);

    // 2. 중심 CoC 계산 (색상용)
    float centerCoC = CalcCoCAtUV(input.texCoord);

    // 3. CoC 블러 (3x3 가우시안) - 경계 부드럽게
    float2 texelSize = ScreenSize.zw;

    const float kernel[3][3] = {
        { 0.0625, 0.125, 0.0625 },
        { 0.125,  0.25,  0.125  },
        { 0.0625, 0.125, 0.0625 }
    };

    float blurredNearCoC = 0.0;
    float blurredFarCoC = 0.0;

    [unroll]
    for (int y = -1; y <= 1; y++)
    {
        [unroll]
        for (int x = -1; x <= 1; x++)
        {
            float2 sampleUV = input.texCoord + float2(x, y) * texelSize;
            float sampleCoC = CalcCoCAtUV(sampleUV);

            float weight = kernel[y + 1][x + 1];

            // Near: 음수 CoC → 양수로 변환해서 누적
            blurredNearCoC += max(-sampleCoC, 0.0) * weight;
            // Far: 양수 CoC 그대로 누적
            blurredFarCoC += max(sampleCoC, 0.0) * weight;
        }
    }

    // 4. Near/Far 분리 출력
    // 색상은 중심 기준, alpha는 블러된 CoC
    output.Near = float4(sceneColor.rgb, blurredNearCoC);
    output.Far = float4(sceneColor.rgb, blurredFarCoC);

    return output;
}
