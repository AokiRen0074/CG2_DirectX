// NeonPlayer.PS.hlsl

cbuffer NeonSettings : register(b1)
{
    float4 neonColor;
    float intensity;
    float radius;
    float softness;
    float dummy; // 16バイトアライメント用
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(VSOutput input) : SV_TARGET
{
    // UVを -1.0 ～ 1.0 に変換し、Y軸を反転）
    float2 p = input.uv * 2.0f - 1.0f;
    p.y = -p.y;

    // ==========================================
    // 自機（正三角形）のSDF（符号付き距離関数）
    // ==========================================
    const float k = 1.7320508f; // sqrt(3)
    p.x = abs(p.x) - 0.5f; // 0.5f が自機のサイズ
    p.y = p.y + 0.5f / k;
    if (p.x + k * p.y > 0.0)
    {
        p = float2(p.x - k * p.y, -k * p.x - p.y) / 2.0f;
    }
    p.x -= clamp(p.x, -1.0f, 0.0f);
    
    // dist は「塗りつぶされた三角形」の距離
    float dist = -length(p) * sign(p.y);

    // 輪郭だけを光らせる（中抜きしてネオン管にする）
    dist = abs(dist) - radius;

    // ==========================================
    // 発光の計算
    // ==========================================
    float core = smoothstep(0.02f, 0.0f, dist);
    float glow = exp(-dist * softness);

    // ベースのオーラ ＋ 強い芯の光
    float3 finalColor = neonColor.rgb * intensity * glow;
    finalColor += neonColor.rgb * core * intensity * 1.5f;

    return float4(finalColor, 1.0f);
}