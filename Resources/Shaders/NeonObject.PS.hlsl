cbuffer NeonSettings : register(b1)
{
    float4 neonColor;
    float intensity;
    float radius;
    float softness;
    float tubeLength; 
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(VSOutput input) : SV_TARGET
{
    // 真ん中から
    float2 p = input.uv * 2.0f - 1.0f;

    // ネオン管の長さ
    float halfLength = tubeLength;
    
    // カプセルからの距離を計算
    float2 d = float2(abs(p.x), max(0.0f, abs(p.y) - halfLength));
    float dist = length(d);

    // 質感の計算
    float core = smoothstep(radius, radius * 0.5f, dist);
    float glow = exp(-dist * softness);

    float3 finalColor = neonColor.rgb * intensity * glow;
    finalColor += core * intensity * 1.5f;

    return float4(finalColor, 1.0f);
}