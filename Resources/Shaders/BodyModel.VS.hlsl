struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

struct VSInput
{
    float4 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 posWorld : TEXCOORD1; // ✨ 追加：世界座標をPSに送る
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.pos = mul(input.pos, gTransformationMatrix.WVP);
    output.uv = input.uv;
    output.posWorld = mul(input.pos, gTransformationMatrix.World).xyz; // ✨
    return output;
}