Texture2D<float4> g_InputTex : register(t0);
SamplerState g_Sampler : register(s0);

// C++から送られてくるパラメータ
cbuffer PostProcessData : register(b0) {
    float g_Time;
    float g_ChromaticAberration;
    float g_NoiseIntensity;
    float g_ToneMapping; // -1: 無効、0: Reinhard、1: ACES近似
    float g_RadialBlur;
    float2 g_BlurCenter;
    float g_BlurPadding;
};

struct VSOutput {
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

// ノイズ生成関数
float rand(float2 co) {
    return frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453);
}

float3 ACESFilm(float3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 main(VSOutput input) : SV_TARGET {
    // 放射状に参照位置をずらし、各サンプルへ色収差も適用する。
    float2 offset = input.uv - g_BlurCenter;
    const int samples = g_RadialBlur > 0.0001f ? 12 : 1;
    float3 color = 0;
    for (int i = 0; i < samples; ++i) {
        float scale = samples > 1 ? 1 - g_RadialBlur * (float(i) / 11) : 1;
        float2 uv = g_BlurCenter + offset * scale;
        float2 shift = (uv - 0.5f) * g_ChromaticAberration;
        if (g_ChromaticAberration > 0) {
            color += float3(g_InputTex.Sample(g_Sampler, uv + shift).r, g_InputTex.Sample(g_Sampler, uv).g,
                            g_InputTex.Sample(g_Sampler, uv - shift).b);
        } else {
            color += g_InputTex.Sample(g_Sampler, uv).rgb;
        }
    }
    color /= samples;

    // フィルムグレイン
    float noise = rand(input.uv * g_Time);
    float noiseFactor = (noise * 2.0f - 1.0f) * g_NoiseIntensity;
    color += color * noiseFactor;

    // HDRの明るさを表示範囲へ圧縮する。モードが無効なら元の値を使う。
    if (g_ToneMapping > 0.5f) {
        color = ACESFilm(color);
    } else if (g_ToneMapping >= 0.0f) {
        color = color / (color + float3(1.0f, 1.0f, 1.0f));
    }

    return float4(color, 1.0f);
}
