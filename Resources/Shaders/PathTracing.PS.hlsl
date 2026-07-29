// C++側から受け取るデータ
cbuffer SceneData : register(b0)
{
    float time;
    float2 resolution;
    float padding;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

// 乱数生成器
float random(inout float seed)
{
    seed = frac(sin(seed) * 43758.5453123);
    return seed;
}

// 球体の交差判定
float2 iSphere(float3 ro, float3 rd, float3 center, float radius, float matID)
{
    float3 oc = ro - center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - radius * radius;
    float h = b * b - c;
    if (h < 0.0)
        return float2(9999.0, -1.0);
    float t = -b - sqrt(h);
    if (t > 0.001)
        return float2(t, matID);
    return float2(9999.0, -1.0);
}

// 平面の交差判定
float2 iPlane(float3 ro, float3 rd, float3 n, float d, float matID)
{
    float t = -(dot(ro, n) + d) / dot(rd, n);
    if (t > 0.001)
        return float2(t, matID);
    return float2(9999.0, -1.0);
}

// 空間全体の判定（コーネルボックス）
float2 map(float3 ro, float3 rd, out float3 normal)
{
    float2 res = float2(9999.0, -1.0);
    float2 hit;

    hit = iPlane(ro, rd, float3(1, 0, 0), 3.0, 1.0); // 左壁 (赤)
    if (hit.x < res.x)
    {
        res = hit;
        normal = float3(1, 0, 0);
    }
    
    hit = iPlane(ro, rd, float3(-1, 0, 0), 3.0, 2.0); // 右壁 (緑)
    if (hit.x < res.x)
    {
        res = hit;
        normal = float3(-1, 0, 0);
    }
    
    hit = iPlane(ro, rd, float3(0, 1, 0), 3.0, 3.0); // 床 (白)
    if (hit.x < res.x)
    {
        res = hit;
        normal = float3(0, 1, 0);
    }
    
    hit = iPlane(ro, rd, float3(0, -1, 0), 3.0, 3.0); // 天井 (白)
    if (hit.x < res.x)
    {
        res = hit;
        normal = float3(0, -1, 0);
    }
    
    hit = iPlane(ro, rd, float3(0, 0, 1), 3.0, 3.0); // 奥壁 (白)
    if (hit.x < res.x)
    {
        res = hit;
        normal = float3(0, 0, 1);
    }

    hit = iSphere(ro, rd, float3(-1.2, -1.5, -1.0), 1.5, 4.0); // 鏡の球
    if (hit.x < res.x)
    {
        res = hit;
        normal = normalize(ro + rd * hit.x - float3(-1.2, -1.5, -1.0));
    }
    
    hit = iSphere(ro, rd, float3(1.2, -2.0, 0.5), 1.0, 3.0); // 白い球
    if (hit.x < res.x)
    {
        res = hit;
        normal = normalize(ro + rd * hit.x - float3(1.2, -2.0, 0.5));
    }

    // 天井のライト（少し広げてノイズを減らす）
    hit = iPlane(ro, rd, float3(0, -1, 0), 2.99, 5.0);
    if (hit.x < res.x)
    {
        float3 p = ro + rd * hit.x;
        if (abs(p.x) < 1.5 && abs(p.z) < 1.5)
        {
            res = hit;
            normal = float3(0, -1, 0);
        }
    }
    return res;
}

float3 getColor(float matID)
{
    if (matID == 1.0)
        return float3(0.8, 0.1, 0.1);
    if (matID == 2.0)
        return float3(0.1, 0.8, 0.1);
    if (matID == 3.0)
        return float3(0.8, 0.8, 0.8);
    if (matID == 4.0)
        return float3(0.9, 0.9, 0.9);
    if (matID == 5.0)
        return float3(12.0, 12.0, 12.0); // ライトの強さ
    return float3(0, 0, 0);
}

// ===============================================
// メイン処理
// ===============================================
float4 main(VSOutput input) : SV_TARGET
{
    float2 p = input.uv * 2.0 - 1.0;
    p.y *= -1.0;
    p.x *= resolution.x / resolution.y;

    float seed = p.x + p.y * 3.431214 + time;
    
    float3 ro = float3(0.0, 0.0, 2.8);
    float3 totalCol = float3(0.0, 0.0, 0.0);

    // ✨ 圧倒的高画質設定（GPUの力でノイズをねじ伏せる）
    const int SAMPLES = 128;
    
    for (int s = 0; s < SAMPLES; s++)
    {
        float2 jitter = float2(random(seed), random(seed)) / resolution.y;
        float3 rd = normalize(float3(p + jitter, -2.5));

        float3 mask = float3(1.0, 1.0, 1.0);
        float3 curRo = ro;
        float3 curRd = rd;

        for (int bounce = 0; bounce < 3; bounce++)
        {
            float3 normal;
            float2 hit = map(curRo, curRd, normal);
            
            if (hit.y < 0.0)
                break;

            float3 hitPos = curRo + curRd * hit.x;
            float matID = hit.y;
            float3 matCol = getColor(matID);

            if (matID == 5.0)
            {
                totalCol += mask * matCol;
                break;
            }

            if (matID == 4.0)
            {
                curRd = reflect(curRd, normal);
                curRo = hitPos + normal * 0.001;
                mask *= matCol;
            }
            else
            {
                // ✨ 修正：コサイン重み付き半球サンプリング（完璧な反射数学）
                float r1 = random(seed) * 6.2831853;
                float r2 = random(seed);
                float z = sqrt(1.0 - r2);
                float xy = sqrt(r2);
                float3 localRay = float3(xy * cos(r1), xy * sin(r1), z);
                
                // 法線に合わせて反射方向を回転させる（TBNマトリクス）
                float3 up = abs(normal.y) < 0.999 ? float3(0, 1, 0) : float3(1, 0, 0);
                float3 tangent = normalize(cross(up, normal));
                float3 bitangent = cross(normal, tangent);
                
                curRd = normalize(tangent * localRay.x + bitangent * localRay.y + normal * localRay.z);
                curRo = hitPos + normal * 0.001;
                mask *= matCol;
            }
        }
    }
    
    float3 finalCol = totalCol / float(SAMPLES);
    finalCol = pow(finalCol, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2)); // ガンマ補正
    
    return float4(finalCol, 1.0);
}