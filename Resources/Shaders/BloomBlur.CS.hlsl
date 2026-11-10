Texture2D<float4> source : register(t0);
Texture2D<float4> horizontal : register(t1);
RWTexture2D<float4> result : register(u0);

cbuffer Settings : register(b0) {
    int vertical;
    float threshold;
    float intensity;
    float padding;
};

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID) {
    uint width, height;
    source.GetDimensions(width, height);
    if (id.x >= width || id.y >= height) {
        return;
    }

    float3 sum = 0;
    float weights = 0;

    // 横・縦の13サンプルに分け、2次元のぼかしに必要な読み取りを減らす。
    for (int i = -6; i <= 6; ++i) {
        int2 offset = vertical == 0 ? int2(i * 2, 0) : int2(0, i * 2);
        int2 p = clamp(int2(id.xy) + offset, int2(0, 0), int2(width, height) - 1);

        // 横パスで明るい成分を抽出し、縦パスでは横ぼかしの結果を読む。
        float3 color = vertical == 0 ? max(source[p].rgb - threshold, 0) : horizontal[p].rgb;
        float weight = exp(-float(i * i) / 15);
        sum += color * weight;
        weights += weight;
    }

    // 重みの合計で正規化し、縦パスで元画像を足して光の芯を残す。
    float3 glow = sum / weights;
    result[id.xy] = float4(vertical == 0 ? glow : source[id.xy].rgb + glow * intensity, 1);
}
