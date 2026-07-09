struct Material
{
    float4 color;
    int enableLighting;
    float3 padding;
    float4x4 uvTransform;

    float3 cameraPos;
    float intensity;
    float radius;
    float3 padding2;
};

ConstantBuffer<Material> gMaterial : register(b0);

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    
    //  VSから来る世界座標を受け取る
    float3 worldPos : TEXCOORD1;
};

float4 main(VSOutput input) : SV_TARGET
{
    //  Player救済ロジック：intensityが0なら従来通りのベタ塗り（これでプレイヤーが復活する）
    if (gMaterial.intensity <= 0.0f)
    {
        return gMaterial.color;
    }

    //  リアルなネオンの計算（フレネル）
    float3 N = normalize(input.normal);
    float3 V = normalize(gMaterial.cameraPos - input.worldPos);
    float facing = abs(dot(N, V));
    float core = pow(facing, max(gMaterial.radius, 0.01f));
    
    float3 baseColor = gMaterial.color.rgb;
    float3 finalColor = lerp(baseColor, float3(1.0f, 1.0f, 1.0f), core);
    
    return float4(finalColor * gMaterial.intensity, gMaterial.color.a);
}