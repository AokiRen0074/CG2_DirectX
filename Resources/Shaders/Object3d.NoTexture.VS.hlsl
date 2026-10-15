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
    float3 worldPos : TEXCOORD1;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.pos = mul(input.pos, gTransformationMatrix.WVP);
    // 法線をワールド空間の向きに変換
    float3x3 world = (float3x3)gTransformationMatrix.World;
    float3x3 normalMatrix = float3x3(cross(world[1], world[2]), cross(world[2], world[0]), cross(world[0], world[1]));
    output.normal = normalize(mul(input.normal, normalMatrix));
    output.worldPos = mul(input.pos, gTransformationMatrix.World).xyz;
    return output;
}