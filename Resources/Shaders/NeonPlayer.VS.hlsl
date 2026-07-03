cbuffer TransformationMatrix : register(b0)
{
    matrix WVP;
};

// ★ここがC++と一致していないとクラッシュします！
struct VSInput
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD; // これが必要！
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.pos = mul(float4(input.pos, 1.0f), WVP);
    output.uv = input.uv;
    return output;
}