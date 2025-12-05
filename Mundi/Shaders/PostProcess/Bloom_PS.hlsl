Texture2D g_Input0 : register(t0);
Texture2D g_Input1 : register(t1);
SamplerState g_LinearClamp : register(s0);

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};

cbuffer BloomSettings : register(b2)
{
    float4 BloomParams0; // x = threshold, y = soft knee, z = intensity, w = mode
    float4 BloomParams1; // x = invWidth, y = invHeight, z = blur radius, w = unused
    float4 BloomParams2; // x = dirX, y = dirY, z = unused, w = unused
}

float3 Prefilter(float2 uv)
{
    float2 texel = BloomParams1.xy;
    float3 halfRes = 0.0f;
    halfRes += g_Input0.Sample(g_LinearClamp, uv + texel * float2(-1.0f, -1.0f)).rgb;
    halfRes += g_Input0.Sample(g_LinearClamp, uv + texel * float2(1.0f, -1.0f)).rgb;
    halfRes += g_Input0.Sample(g_LinearClamp, uv + texel * float2(1.0f, 1.0f)).rgb;
    halfRes += g_Input0.Sample(g_LinearClamp, uv + texel * float2(-1.0f, 1.0f)).rgb;
    halfRes *= 0.25f;

    float threshold = BloomParams0.x;
    float softKnee = BloomParams0.y;
    float brightness = max(max(halfRes.r, halfRes.g), halfRes.b);
    float knee = max(threshold * softKnee, 1e-5f);
    float soft = saturate((brightness - threshold + knee) / (knee));
    float contribution = soft * soft;
    return halfRes * contribution;
}

float3 Blur(float2 uv)
{
    float2 texel = BloomParams1.xy * max(BloomParams1.z, 0.5f);
    float2 dir = BloomParams2.xy;
    float2 step = texel * dir;

    float3 color = g_Input0.Sample(g_LinearClamp, uv).rgb * 0.227027f;
    color += g_Input0.Sample(g_LinearClamp, uv + step * 1.384615f).rgb * 0.316216f;
    color += g_Input0.Sample(g_LinearClamp, uv - step * 1.384615f).rgb * 0.316216f;
    color += g_Input0.Sample(g_LinearClamp, uv + step * 3.230769f).rgb * 0.070270f;
    color += g_Input0.Sample(g_LinearClamp, uv - step * 3.230769f).rgb * 0.070270f;
    return color;
}

float3 CompositeColor(float2 uv)
{
    float3 scene = g_Input0.Sample(g_LinearClamp, uv).rgb;
    float3 bloom = g_Input1.Sample(g_LinearClamp, uv).rgb;
    return scene + bloom * BloomParams0.z;
}

float4 mainPS(PS_INPUT input) : SV_Target
{
    if (BloomParams0.w < 0.5f)
    {
        return float4(Prefilter(input.TexCoord), 1.0f);
    }
    else if (BloomParams0.w < 1.5f)
    {
        return float4(Blur(input.TexCoord), 1.0f);
    }
    else
    {
        return float4(CompositeColor(input.TexCoord), 1.0f);
    }
}
