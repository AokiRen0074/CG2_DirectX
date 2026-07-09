struct Material
{
    float4 color;
    int enableLighting;
    float3 padding;
    float4x4 uvTransform;
};

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float3 posWorld : TEXCOORD1;
};

float4 main(VSOutput input) : SV_TARGET
{
    float4 texColor = gTexture.Sample(gSampler, input.uv);
    // これがImGuiの暗い紫 (R:26, G:13, B:38)
    float4 baseColor = gMaterial.color * texColor;

    // 面から法線を生成
    float3 dx = ddx(input.posWorld);
    float3 dy = ddy(input.posWorld);
    float3 N = normalize(cross(dx, dy));
    
    float3 V = normalize(float3(0.0f, 0.0f, -1.0f));
    if (dot(N, V) < 0.0f)
    {
        N = -N;
    }

    float3 L = normalize(-gDirectionalLight.direction);
    float3 lightColor = gDirectionalLight.color.rgb * gDirectionalLight.intensity;
    
    // 1. 環境光（宇宙の暗さをキープ！）
    float3 ambient = baseColor.rgb * 0.2f;
    
    // 2. ディフューズ（光が当たった面）
    // 白飛びしないように、baseColorをベースにして少し明るくする程度に留める
    float NdotL = max(dot(N, L), 0.0f);
    float3 diffuse = baseColor.rgb * NdotL * lightColor * 0.8f;
    
    // ✨ 3. スペキュラー（目標画像のような鋭い光沢）
    float3 H = normalize(L + V);
    float NdotH = max(dot(N, H), 0.0f);
    // 0.7～0.95 の間でスパッと白くさせる（ローポリ特化のパキッとした反射）
    float specIntensity = smoothstep(0.7f, 0.95f, NdotH);
    float3 specular = float3(1.0f, 0.9f, 1.0f) * specIntensity * 1.5f;
    
    // 最終合成
    float3 finalColor = ambient + diffuse + specular;

    return float4(saturate(finalColor), baseColor.a);
}