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
    
    // 視線ベクトルの計算が可能になった！
    float3 V = normalize(gLight.cameraPos - input.worldPos);
    
    // --- ① 平行光源（全体を照らす弱い光） ---
    float wrapDirLight = dot(N, -gLight.dirDirection) * 0.5f + 0.5f;
    float diffuseLight = wrapDirLight * wrapDirLight; // 点光源と同じく2乗してコントラストを整える
    
    float ambient = 0.4f;
    diffuseLight = diffuseLight * (1.0f - ambient) + ambient;
    
    float3 diffuse = baseColor * diffuseLight * gLight.dirColor.rgb * gLight.dirIntensity;
    
    float3 pointToLight = gLight.pointPos - input.worldPos;
    float distance = length(pointToLight);
    float3 L_point = normalize(pointToLight);
    
    // 距離による減衰
    float attenuation = max(0.0f, 1.0f - (distance / max(gLight.pointRadius, 0.01f)));
    attenuation = pow(attenuation, 2.0f);

    float wrapLight = dot(N, L_point) * 0.5f + 0.5f;
    float pointDiffuseLight = wrapLight * wrapLight; // 2乗してコントラストを整える
    
    // ボディの元の色(暗い紫)にそのまま掛けると光が弱すぎるため、ネオンの光が勝つように調整
    float3 litBaseColor = baseColor + float3(0.2f, 0.2f, 0.2f);
    float3 pointDiffuse = litBaseColor * pointDiffuseLight * gLight.pointColor.rgb * gLight.pointIntensity * attenuation;
    
    // ツヤ（スペキュラ）の計算
    float3 R_point = reflect(-L_point, N);
    float pointSpecularLight = pow(max(dot(R_point, V), 0.0f), 16.0f);
    float3 pointSpecular = float3(1.0f, 1.0f, 1.0f) * pointSpecularLight * gLight.pointColor.rgb * gLight.pointIntensity * attenuation;

    // すべての光を合成！
    float3 finalColor = diffuse + pointDiffuse + pointSpecular;
    
    return float4(finalColor, gMaterial.color.a);
}