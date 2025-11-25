

// 빔 전용 버텍스 셰이더
  struct VSInput
  {
      float3 Position : POSITION;
      float2 UV : TEXCOORD0;
      float4 Color : COLOR;
  };

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 UV : TEXCOORD0;
    float4 Color : COLOR;
};

PSInput VS_Main(VSInput input)
{
    PSInput output;
    output.Position = mul(float4(input.Position, 1.0), WorldViewProjection);
    output.UV = input.UV;
    output.Color = input.Color;
    return output;
}

// 빔 전용 픽셀 셰이더
Texture2D BeamTexture : register(t0);
SamplerState BeamSampler : register(s0);

float4 PS_Main(PSInput input) : SV_TARGET
{
    float4 texColor = BeamTexture.Sample(BeamSampler, input.UV);
    return texColor * input.Color;
}