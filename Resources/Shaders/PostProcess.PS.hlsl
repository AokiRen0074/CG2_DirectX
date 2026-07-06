// SRV (先ほど書き込んだ結果のテクスチャ)
Texture2D<float4> g_InputTex : register(t0);
// 画像を滑らかに読み込むためのサンプラー
SamplerState g_Sampler : register(s0);

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(VSOutput input) : SV_TARGET
{
    // コンピュートシェーダーで作った画像から色を読み取る
    float4 color = g_InputTex.Sample(g_Sampler, input.uv);

    // 【トーンマッピング】
    // そのまま表示すると1.0以上の色が白飛びして汚くなるため、
    // Reinhard（ラインハルト）という計算式で 0.0 ～ 1.0 の範囲に滑らかに圧縮します
    color.rgb = color.rgb / (color.rgb + float3(1.0f, 1.0f, 1.0f));

    return float4(color.rgb, 1.0f);
}