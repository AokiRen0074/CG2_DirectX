struct Material
{
    float4 color;
    int enableLighting;
    float3 padding;
    float4x4 uvTransform;
};

// 🌟 C++と完全に一致させたライト構造体
struct LightData
{
    float4 dirColor;
    float3 dirDirection;
    float dirIntensity;
    
    float3 pointPos;
    float pointIntensity;
    float4 pointColor;
    
    float pointRadius;
    float3 cameraPos; // 🌟 追加されたカメラ座標
};

ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<LightData> gLight : register(b1);
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
    float3 baseColor = gMaterial.color.rgb * texColor.rgb;
    
    if (gMaterial.enableLighting == 0)
    {
        return float4(baseColor, gMaterial.color.a);
    }

    float3 N = normalize(input.normal);
    
    // 🌟 視線ベクトルの計算が可能になった！
    float3 V = normalize(gLight.cameraPos - input.worldPos);
    
    // --- ① 平行光源（全体を照らす弱い光） ---
    float diffuseLight = max(dot(N, -gLight.dirDirection), 0.0f);
    float3 diffuse = baseColor * diffuseLight * gLight.dirColor.rgb * gLight.dirIntensity;
    
    // --- ② 🌟 ネオンからの照り返し（点光源） ---
    float3 pointToLight = gLight.pointPos - input.worldPos;
    float distance = length(pointToLight);
    float3 L_point = normalize(pointToLight);
    
    // 距離による減衰（近づくほど強い光になる）
    float attenuation = max(0.0f, 1.0f - (distance / max(gLight.pointRadius, 0.01f)));
    attenuation = pow(attenuation, 2.0f);
    
    // ボディへの照り返し（ディフューズ：色がフワッと乗る）
    float pointDiffuseLight = max(dot(N, L_point), 0.0f);
    float3 pointDiffuse = baseColor * pointDiffuseLight * gLight.pointColor.rgb * gLight.pointIntensity * attenuation;
    
    // 🌟 ボディのツヤ（ネオンの光が金属に鋭く反射するスペキュラ！）
    float3 R_point = reflect(-L_point, N);
    float pointSpecularLight = pow(max(dot(R_point, V), 0.0f), 16.0f);
    
    // スペキュラは白やネオンの原色で強く光らせる
    float3 pointSpecular = float3(1.0f, 1.0f, 1.0f) * pointSpecularLight * gLight.pointColor.rgb * gLight.pointIntensity * attenuation;

    // すべての光を合成！
    float3 finalColor = diffuse + pointDiffuse + pointSpecular;
    
    return float4(finalColor, gMaterial.color.a);
}