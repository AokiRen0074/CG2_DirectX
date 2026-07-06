Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(VSOutput input) : SV_TARGET
{
    float4 color = gTexture.Sample(gSampler, input.uv);
    
    // 輝度の計算
    float luminance = dot(color.rgb, float3(0.299f, 0.587f, 0.114f));
    
    // しきい値を超えた部分だけを抽出して、さらに強く光らせる
    float threshold = 0.8f;
    if (luminance > threshold)
    {
        return float4(color.rgb * 10.0f, 1.0f); // 10倍の強度（HDR）で出力
    }
    
    return float4(0, 0, 0, 1); // 暗いところは真っ黒にする
}

