struct Material {
    float4 color;
    int enableLighting;
    float3 padding;
    float4x4 uvTransform;
};
struct VertexShaderOutput {
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPos : TEXCOORD1;
};
ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);
float4 main(VertexShaderOutput input) : SV_TARGET {
    float2 uv = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform).xy;
    return gMaterial.color * gTexture.Sample(gSampler, uv);
}