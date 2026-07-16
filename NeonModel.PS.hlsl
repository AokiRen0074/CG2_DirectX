struct Material
{
    float4 color;
    int enableLighting;
    float3 padding;
    float4x4 uvTransform;

    float3 cameraPos;
    float intensity;
    float radius;
    
    float time;
    float usePlasma;
    float padding2;
};

ConstantBuffer<Material> gMaterial : register(b0);

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0; 
    float3 normal : NORMAL0;
    float3 worldPos : TEXCOORD1;
};

float4 main(VSOutput input) : SV_TARGET
{
    // ==========================================
    // Playerネオンの計算
    // ==========================================
    if (gMaterial.intensity <= 0.0f)
    {
        float3 glowColor = gMaterial.color.rgb;
        
        if (gMaterial.usePlasma > 0.5f)
        {
            // UVのY座標と時間を使って、波を作る
            float plasma = sin(input.texcoord.y * 15.0f - gMaterial.time * 10.0f);
            // 波
            plasma = (plasma * 0.5f + 0.5f) * 0.5f + 0.5f;
            
            glowColor *= plasma; // 光に波を掛け算
        }
        
        return float4(glowColor, gMaterial.color.a);
    }

    // ==========================================
    // ネオン看板用の計算
    // ==========================================
    
    // 法線
    float3 N = normalize(input.normal);
    
    // カメラの向き
    float3 V = normalize(gMaterial.cameraPos - input.worldPos);
    
    // 面がカメラを向いている度合
    float facing = abs(dot(N, V));
    
    // 芯とオーラを分離
    float power = 1.0f / max(gMaterial.radius, 0.001f);
    float core = pow(facing, power);
    
    // フチのネオンカラー
    float3 glowColor = gMaterial.color.rgb * gMaterial.intensity;
    
    // ネオン看板にプラズマを流す！
    if (gMaterial.usePlasma > 0.5f)
    {
        // UVのX座標　管に沿って流れる波
        float plasma = sin(input.texcoord.x * 20.0f - gMaterial.time * 15.0f);
        // 波
        plasma = (plasma * 0.5f + 0.5f) * 0.6f + 0.4f;
        
        glowColor *= plasma; // 光に波を掛け算
    }

    // 芯の純白（白飛び）
    float3 coreColor = float3(1.0f, 1.0f, 1.0f) * (core * 2.0f);
    
    // 最終的な色
    float3 finalColor = glowColor + coreColor;
   
    
    
    return float4(finalColor, gMaterial.color.a);
}