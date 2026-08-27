// ==========================================
// 構造体の定義
// ==========================================

// マテリアル
struct Material
{
    float32_t4 color;
    int32_t enableLighting;
    float32_t3 padding;
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
    float32_t3 worldPos : TEXCOORD1;
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
    float32_t4 transformedUV = mul(float32_t4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float32_t4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    
    
    
    float32_t4 outputColor;

    if (gMaterial.enableLighting != 0)
    {
        float32_t3 normal = normalize(input.normal);
        float32_t3 lightDir = normalize(-gDirectionalLight.direction);
        
        //  拡散反射
        float NdotL = dot(normal, lightDir);
        float diffuse = pow(NdotL * 0.5f + 0.5f, 2.0f);
        
        // 鏡面反射
        float32_t3 viewDir = normalize(float32_t3(0.0f, 0.0f, -1.0f));
        float32_t3 halfVector = normalize(lightDir + viewDir);
        float NdotH = dot(normal, halfVector);
        
        // ハイライトの鋭さ
        float specular = pow(max(NdotH, 0.0f), 40.0f);
        
        float32_t3 baseColor = gMaterial.color.rgb * textureColor.rgb;
        
        outputColor.rgb = (baseColor * diffuse * gDirectionalLight.color.rgb.xyz * gDirectionalLight.intensity)
                        + (baseColor * specular * 3.0f * gDirectionalLight.intensity);
        
        outputColor.a = gMaterial.color.a * textureColor.a;
    }
    else
    {
        outputColor = gMaterial.color * textureColor;
    }

    return outputColor;
}