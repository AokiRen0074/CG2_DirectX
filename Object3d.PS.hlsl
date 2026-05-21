// ==========================================
// 構造体の定義
// ==========================================

// マテリアル
struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    float32_t4x4 uvTransform;
};

// 平行光源
struct DirectionalLight
{
    float32_t4 color; // ライトの色
    float32_t3 direction; // ライトの向き
    float intensity; // 輝度
};

// 頂点シェーダーから送られてきたデータ
struct VertexShaderOutput
{
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
};

// ==========================================
// 定数バッファとリソース
// ==========================================
ConstantBuffer<Material> gMaterial : register(b0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);
Texture2D<float32_t4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// ==========================================
// メイン関数
// ==========================================
float32_t4 main(VertexShaderOutput input) : SV_TARGET
{
    
    // テクスチャから色をサンプリング
    float32_t4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    

    float32_t4 outputColor;

    // ライティングの計算
    if (gMaterial.enableLighting != 0)
    {
        // ライティング有効の場合
        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
 
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
        // 光の計算
        outputColor.rgb = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
        
  
        outputColor.a = gMaterial.color.a * textureColor.a;
        
    }
    else
    {
        // ライティング無効の場合
        outputColor = gMaterial.color * textureColor;
    }

    return outputColor;
}