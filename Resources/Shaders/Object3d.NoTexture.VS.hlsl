struct TransformationMatrix
{
    matrix WVP;
    matrix World;
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
    float3 normal : NORMAL;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.pos = mul(input.pos, gTransformationMatrix.WVP);
    // 法線をワールド空間の向きに変換
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.World));
    return output;
}