RWTexture2D<float4> accumTex : register(u0);

cbuffer SceneData : register(b0)
{
    float time;
    float2 resolution;
    float frameCount;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

/*------------------------
乱数生成
----------------------------*/
uint pcg_hash(uint input)
{
    uint state = input * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

float random(inout uint seed)
{
    seed = pcg_hash(seed);
    return float(seed) / 4294967295.0; 
}

/*--------------------------------
ACESトーンマッピング
----------------------------------*/
float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
}

//形状の交差判定
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

float2 iPlane(float3 ro, float3 rd, float3 n, float d, float matID)
{
    float t = -(dot(ro, n) + d) / dot(rd, n);
    if (t > 0.001)
        return float2(t, matID);
    return float2(9999.0, -1.0);
}

/*--------------------
コーネルボックス
-------------------------------*/
float2 map(float3 ro, float3 rd, out float3 normal)
{
    float2 res = float2(9999.0, -1.0);
    float2 hit;


    hit = iPlane(ro, rd, float3(1, 0, 0), 1.0, 1.0); // 左壁 
    if (hit.x < res.x)
    {
        res = hit;
        normal = float3(1, 0, 0);
    }
    hit = iPlane(ro, rd, float3(-1, 0, 0), 1.0, 2.0); // 右壁 
    if (hit.x < res.x)
    {
        res = hit;
        normal = float3(-1, 0, 0);
    }
    hit = iPlane(ro, rd, float3(0, 1, 0), 1.0, 3.0); // 床 
    if (hit.x < res.x)
    {
        res = hit;
        normal = float3(0, 1, 0);
    }
    hit = iPlane(ro, rd, float3(0, -1, 0), 1.0, 3.0); // 天井
    if (hit.x < res.x)
    {
        res = hit;
        normal = float3(0, -1, 0);
    }
    hit = iPlane(ro, rd, float3(0, 0, -1), 1.0, 3.0); // 奥壁
    if (hit.x < res.x)
    {
        res = hit;
        normal = float3(0, 0, -1);
    }

    // 左の鏡の球
    hit = iSphere(ro, rd, float3(-0.4, -0.6, 0.2), 0.4, 4.0);
    if (hit.x < res.x)
    {
        res = hit;
        normal = normalize(ro + rd * hit.x - float3(-0.4, -0.6, 0.2));
    }
    
    // 右の白い球
    hit = iSphere(ro, rd, float3(0.4, -0.6, -0.3), 0.4, 3.0);
    if (hit.x < res.x)
    {
        res = hit;
        normal = normalize(ro + rd * hit.x - float3(0.4, -0.6, -0.3));
    }

    // 天井のライト
    hit = iPlane(ro, rd, float3(0, -1, 0), 0.999, 5.0);
    if (hit.x < res.x)
    {
        float3 p = ro + rd * hit.x;
        if (abs(p.x) < 0.3 && abs(p.z) < 0.3)
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
        return float3(1.0, 1.0, 1.0); // 鏡の反射率
    if (matID == 5.0)
        return float3(8.0, 8.0, 8.0); // ライトの強さ
    return float3(0, 0, 0);
}

// ===============================================
// メイン処理
// ===============================================
float4 main(VSOutput input) : SV_TARGET
{
    uint2 pixelCoord = uint2(input.pos.xy);
    
    // 画面座標を正しく取得
    float2 p = (float2(pixelCoord) / resolution.xy) * 2.0 - 1.0;
    p.y *= -1.0;
    p.x *= resolution.x / resolution.y;

    // 乱数のシードを生成
    uint seed = pixelCoord.y * uint(resolution.x) + pixelCoord.x + uint(frameCount) * 719393u;
    
    float3 ro = float3(0.0, 0.0, -2.8);
    float3 totalCol = float3(0.0, 0.0, 0.0);

    // サンプル数
    const int SAMPLES = 128; 
    
    for (int s = 0; s < SAMPLES; s++)
    {
        // ピクセル内でわずかに発射位置をズラす（
        float2 jitter = float2(random(seed) - 0.5, random(seed) - 0.5) / resolution.y;
        float3 rd = normalize(float3(p + jitter, 2.0)); // 手前から奥へ飛ばす

        float3 mask = float3(1.0, 1.0, 1.0);
        float3 curRo = ro;
        float3 curRd = rd;

        for (int bounce = 0; bounce < 4; bounce++)
        { // 4回バウンド
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
                break; // 光源に当たったら終了
            }

            if (matID == 4.0)
            {
                curRd = reflect(curRd, normal);
                curRo = hitPos + normal * 0.001;
                mask *= matCol;
            }
            else
            {
                float r1 = random(seed) * 6.2831853;
                float r2 = random(seed);
                float z = sqrt(1.0 - r2);
                float xy = sqrt(r2);
                float3 localRay = float3(xy * cos(r1), xy * sin(r1), z);
                
                float3 up = abs(normal.y) < 0.999 ? float3(0, 1, 0) : float3(1, 0, 0);
                float3 tangent = normalize(cross(up, normal));
                float3 bitangent = cross(normal, tangent);
                
                curRd = normalize(tangent * localRay.x + bitangent * localRay.y + normal * localRay.z);
                curRo = hitPos + normal * 0.001;
                mask *= matCol;
            }
        }
    }
    
    float3 newCol = totalCol / float(SAMPLES);
    
    // 時間的蓄積
    float3 oldCol = accumTex[pixelCoord].rgb;
    float blendFactor = 1.0f / max(frameCount, 1.0f);
    float3 blendedCol = lerp(oldCol, newCol, blendFactor);
    
    // キャンバスに上書き保存
    accumTex[pixelCoord] = float4(blendedCol, 1.0f);
    
    float3 finalCol = ACESFilm(blendedCol);
    finalCol = pow(finalCol, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
    
    return float4(finalCol, 1.0f);
}