#include "DOF_Common.hlsli"

// Input Texture
Texture2D    g_InputTex : register(t0);
SamplerState g_LinearClampSample : register(s0);

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

// Constant Buffer
cbuffer DOFBlurCB : register(b2)
{
    float2 BlurDirection;      // (1,0) for horizontal, (0,1) for vertical
    float  BlurRadius;         // pixels (최대 블러 반경)
    float  _Pad0;
}

cbuffer ViewportConstants : register(b10)
{
    float4 ViewportRect;
    float4 ScreenSize;
}

// 출력
struct PS_OUTPUT
{
    float4 Color : SV_Target0;
};

// Separable Gaussian Blur (Variable Kernel Size)
PS_OUTPUT mainPS(PS_INPUT input)
{
    PS_OUTPUT output;

    // 1. 중앙 샘플
    float4 centerSample = g_InputTex.Sample(g_LinearClampSample, input.texCoord);
    float coc = centerSample.a;  // Alpha 채널에 CoC 저장되어 있음

    // 2. CoC가 0이면 블러 불필요 (선명 영역)
    if (coc < 0.001)
    {
        output.Color = centerSample;
        return output;
    }

    // 3. Variable Kernel Size (CoC에 비례)
    // CoC는 0~1 정규화 값이므로 BlurRadius를 곱해 픽셀 단위로 변환
    float kernelRadius = coc * BlurRadius;  // 0~1 * MaxBlurSize = 픽셀
    kernelRadius = max(kernelRadius, 1.0);  // 최소 1 픽셀

    // 4. Gaussian Blur (13-tap)
    float2 texelSize = ScreenSize.zw;  // 1.0 / ScreenSize
    float2 blurStep = BlurDirection * texelSize;

    float4 colorSum = float4(0, 0, 0, 0);
    float weightSum = 0.0;

    // Center
    colorSum += centerSample * GAUSSIAN_WEIGHTS_13[0];
    weightSum += GAUSSIAN_WEIGHTS_13[0];

    // ±1 ~ ±6
    [unroll]
    for (int i = 1; i < 7; i++)
    {
        float offset = float(i) * (kernelRadius / 6.0);  // 스케일링
        float2 sampleOffset = blurStep * offset;
        float weight = GAUSSIAN_WEIGHTS_13[i];

        // Positive direction
        float4 samplePos = g_InputTex.Sample(g_LinearClampSample, input.texCoord + sampleOffset);
        colorSum += samplePos * weight;
        weightSum += weight;

        // Negative direction
        float4 sampleNeg = g_InputTex.Sample(g_LinearClampSample, input.texCoord - sampleOffset);
        colorSum += sampleNeg * weight;
        weightSum += weight;
    }

    // 5. Normalize
    output.Color = colorSum / weightSum;

    return output;
}
