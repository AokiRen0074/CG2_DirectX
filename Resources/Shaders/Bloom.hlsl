// Bloom.hlsl

// SRV 
Texture2D<float4> g_InputTex : register(t0);

// UAV
RWTexture2D<float4> g_OutputTex : register(u0);

//  C++からON/OFFのスイッチを受け取る定数バッファを追加！
cbuffer BloomSettings : register(b0)
{
    int enableLuminance;
    int enableBlur;
    int enableAdditive;
    float padding; // 16バイトに合わせるための余白
};

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 pos = dispatchThreadID.xy;

    uint width, height;
    g_InputTex.GetDimensions(width, height);

    if (pos.x >= width || pos.y >= height)
        return;

    // 元の画像のピクセル
    float4 baseColor = g_InputTex[pos];

    float4 bloomColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float totalWeight = 0.0f;

    //  ぼかしがOFFの時は、ループ範囲を0にして重い処理を完全にスキップする！
    int blurRadius = (enableBlur == 1) ? 6 : 0;

    for (int y = -blurRadius; y <= blurRadius; ++y)
    {
        for (int x = -blurRadius; x <= blurRadius; ++x)
        {
            int2 samplePos = int2(pos.x + x, pos.y + y);
            samplePos.x = clamp(samplePos.x, 0, width - 1);
            samplePos.y = clamp(samplePos.y, 0, height - 1);

            float4 sampleColor = g_InputTex[samplePos];

            // 輝度抽出のON/OFF
            float4 extractColor = sampleColor;
            if (enableLuminance == 1)
            {
                // 1.0を超えている「強い光」の部分だけを抜き出す
                extractColor = max(sampleColor - 1.0f, 0.0f);
            }

            // ぼかしの計算（重み付け）
            float distanceSq = (float) (x * x + y * y);
            float weight = exp(-distanceSq / 15.0f);

            // ぼかしOFFの時は重みを1.0にしてそのまま足す
            if (enableBlur == 0)
            {
                weight = 1.0f;
            }

            bloomColor += extractColor * weight;
            totalWeight += weight;
        }
    }

    bloomColor /= totalWeight;

    // ぼかした光の強さを増幅させる
    bloomColor *= 1.5f;
    
    // 加算合成のON/OFF
    float4 finalColor = float4(0.0f, 0.0f, 0.0f, 1.0f);
    if (enableAdditive == 1)
    {

        finalColor = baseColor + bloomColor;
    }
    else
    {

        finalColor = bloomColor;
    }

    g_OutputTex[pos] = float4(finalColor.rgb, 1.0f);
}