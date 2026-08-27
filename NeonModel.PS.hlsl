// 🌟 修正：一切の無駄を省き、ピッタリ96バイトに収めた最強の構造体
struct Material
{
    float4 color; // 16バイト
    int enableLighting; // 4バイト
    float intensity; // 4バイト
    float time; // 4バイト
    float usePlasma; // 4バイト
    float4x4 uvTransform; // 64バイト
};

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPos : TEXCOORD1;
};

float4 main(VSOutput input) : SV_TARGET
{
    float4 texColor = gTexture.Sample(gSampler, input.texcoord);

    if (texColor.a <= 0.1f)
    {
        discard;
    }

    // ==========================================
    // ネオン・ロックオンUIの計算
    // ==========================================
    if (gMaterial.intensity <= 0.0f)
    {
        float3 glowColor = gMaterial.color.rgb * texColor.rgb;
        if (gMaterial.usePlasma > 0.5f)
        {
            float plasma = sin(input.texcoord.y * 15.0f - gMaterial.time * 10.0f);
            plasma = (plasma * 0.5f + 0.5f) * 0.5f + 0.5f;
            glowColor *= plasma;
        }
        return float4(glowColor, gMaterial.color.a * texColor.a);
    }

    // ==========================================
    // ネオン看板用の計算
    // ==========================================
    float3 glowColor = gMaterial.color.rgb * gMaterial.intensity;
    
    if (gMaterial.usePlasma > 0.5f)
    {
        float plasma = sin(input.texcoord.x * 20.0f - gMaterial.time * 15.0f);
        plasma = (plasma * 0.5f + 0.5f) * 0.6f + 0.4f;
        glowColor *= plasma;
    }

    // 複雑なカメラ計算を捨ててシンプル化！
    float3 coreColor = float3(1.0f, 1.0f, 1.0f) * 2.0f * texColor.rgb;
    float3 finalColor = glowColor + coreColor;
   
    return float4(finalColor, gMaterial.color.a);
}