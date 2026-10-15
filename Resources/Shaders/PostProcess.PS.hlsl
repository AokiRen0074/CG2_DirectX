Texture2D<float4> g_InputTex : register(t0);
SamplerState g_Sampler : register(s0);

// C++から送られてくるパラメータ
cbuffer PostProcessData : register(b0)
{
    float g_Time;
    float g_ChromaticAberration;
    float g_NoiseIntensity;
    float g_UseACES; // 🌟 追加：切り替えフラグを受け取る！
    float g_RadialBlur;
    float2 g_BlurCenter;
    float g_BlurPadding;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

// ノイズ生成関数
float rand(float2 co)
{
    return frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453);
}

// 映画業界標準のACESトーンマッピング関数
float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 main(VSOutput input) : SV_TARGET
{
    /*--------------------------
    色収差の計算
    --------------------------*/
    // 画面の中心からの距離とベクトル
    float2 centerOffset = input.uv - 0.5f;
    
    // 赤と青のUV座標を外側へずらす
    float2 uvR = input.uv + centerOffset * g_ChromaticAberration;
    float2 uvG = input.uv;
    float2 uvB = input.uv - centerOffset * g_ChromaticAberration;

    // ずらした別々のUV座標を使い、RGBを別々に読み込む
    float r = g_InputTex.Sample(g_Sampler, uvR).r;
    float g = g_InputTex.Sample(g_Sampler, uvG).g;
    float b = g_InputTex.Sample(g_Sampler, uvB).b;

    float3 color = float3(r, g, b);

    // 撃破位置を中心に短く広がるブラー。0の間はサンプリングを追加しない。
    if (g_RadialBlur > 0.0001f)
    {
        float2 offset = input.uv - g_BlurCenter;
        float3 blurred = float3(0.0f, 0.0f, 0.0f);

        [unroll]
        for (int i = 0; i < 12; ++i)
        {
            float scale = 1.0f - g_RadialBlur * (float(i) / 11.0f);
            blurred += g_InputTex.Sample(g_Sampler, g_BlurCenter + offset * scale).rgb;
        }

        color = blurred / 12.0f;
    }

    // フィルムグレイン
    float noise = rand(input.uv * g_Time);
    float noiseFactor = (noise * 2.0f - 1.0f) * g_NoiseIntensity;
    color += color * noiseFactor;

    // ==========================================
    // 3. トーンマッピングのリアルタイム切り替え！
    // ==========================================
    if (g_UseACES > 0.5f)
    {
        // 🌟 チェックボックスがONなら：映画用の ACES カーブを通す（美しい白飛び）
        color = ACESFilm(color);
    }
    else if (g_UseACES >= 0.0f)
    {
        // 🌟 チェックボックスがOFFなら：従来の Reinhard カーブを通す（くすんだ色）
        color = color / (color + float3(1.0f, 1.0f, 1.0f));
    }

    return float4(color, 1.0f);
}
