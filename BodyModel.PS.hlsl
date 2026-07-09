struct Material
{
    float4 color;
    int enableLighting;
    float3 padding;
    float4x4 uvTransform;
};

ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 posWorld : TEXCOORD1; // ✨ 受け取る
};

float4 main(VSOutput input) : SV_TARGET
{
    float4 texColor = gTexture.Sample(gSampler, input.uv);
    float4 baseColor = gMaterial.color * texColor;

    // ==========================================
    // ✨ 魔法の計算：ポリゴンの面から「正しい法線」を自動生成！
    // ==========================================
    float3 dx = ddx(input.posWorld);
    float3 dy = ddy(input.posWorld);
    float3 N = normalize(cross(dx, dy));
    
    // 面が裏返って暗くなるのを防ぐため、常にカメラ側を向くように補正
    if (dot(N, float3(0.0f, 0.0f, -1.0f)) < 0.0f)
    {
        N = -N;
    }

    // 光とカメラの向き
    float3 L = normalize(float3(-1.0f, 1.0f, -1.0f));
    float3 V = normalize(float3(0.0f, 0.0f, -1.0f));
    
    // 1. 環境光
    float3 ambient = baseColor.rgb * 0.05f;
    
    // 2. ディフューズ（基本の明るさ）
    float NdotL = max(dot(N, L), 0.0f);
    float3 diffuse = baseColor.rgb * NdotL * 1.5f;
    
    // 3. スペキュラー（広い光沢！）
    float3 H = normalize(L + V);
    float NdotH = max(dot(N, H), 0.0f);
    float specIntensity = pow(NdotH, 4.0f);
    float3 specular = float3(1.0f, 0.9f, 1.0f) * specIntensity * 2.0f;
    
    // 4. リムライト（エッジのハイライト）
    float rim = 1.0f - max(dot(V, N), 0.0f);
    rim = smoothstep(0.6f, 1.0f, rim);
    float3 rimColor = float3(0.8f, 0.3f, 0.8f) * rim;
    
    // 最終合成
    float3 finalColor = ambient + diffuse + specular + rimColor;

    return float4(saturate(finalColor), baseColor.a);
}