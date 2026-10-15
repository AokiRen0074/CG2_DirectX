struct Input {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    nointerpolation float shape : TEXCOORD1;
};

float Hash(float2 p) {
    return frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453);
}

float Noise(float2 p) {
    float2 cell = floor(p);
    float2 t = frac(p);
    t = t * t * (3 - 2 * t);
    return lerp(lerp(Hash(cell), Hash(cell + float2(1, 0)), t.x),
                lerp(Hash(cell + float2(0, 1)), Hash(cell + 1), t.x), t.y);
}

float4 main(Input input) : SV_TARGET {
    if (input.shape >= 3) {
        // 実距離のUVに模様を流し、四角い帯の中に不規則なプラズマを作る。
        float time = (input.shape - 3) * 2048 / 60;
        float2 p = float2(input.uv.x * 3.5 - time * 22, input.uv.y * 2.8);
        float turbulence = Noise(p) * 0.65 + Noise(p * 2.3 + time * 3) * 0.35;
        float edge = abs(input.uv.y) + (turbulence - 0.5) * 0.3;
        float mask = 1 - smoothstep(0.55, 1, edge);
        float core = 1 - smoothstep(0.05 + turbulence * 0.12, 0.4 + turbulence * 0.2, edge);
        float knots = 0.7 + Noise(float2(input.uv.x * 2 - time * 17, time)) * 0.6;
        float3 heat = lerp(float3(3.5, 0.08, 0.04), float3(6, 3.3, 1.8), core);
        return float4(heat * mask * knots * input.color.a, 0);
    }

    // 素材の四角い輪郭を出さず、芯から外へ滑らかに透明にする。
    float across = saturate(1 - abs(input.uv.y));
    float mask = across * across;
    if (input.shape > 1.5) {
        mask = 1;
    }

    if (input.shape < 0.5) {
        float2 p = float2(input.uv.x * 2 - 1, input.uv.y);
        mask = pow(saturate(1 - dot(p, p)), 3);
    }

    // RGBをアルファで乗算してから加算。明るさ1以上をHDRに保存する。
    return float4(input.color.rgb * input.color.a * mask, 0);
}
