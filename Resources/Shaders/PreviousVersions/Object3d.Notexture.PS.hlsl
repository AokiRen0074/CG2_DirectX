struct Material
{
    float4 color;
    int enableLighting;
    float3 padding;
    float4x4 uvTransform;
};
ConstantBuffer<Material> gMaterial : register(b0);

struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
    int lightingType;
    float3 padding;
};

ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
};

float4 main(VSOutput input) : SV_TARGET
{
    float4 outputColor = gMaterial.color;

    if (gMaterial.enableLighting != 0)
    {
        float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
        
        if (gDirectionalLight.lightingType == 1)
        {
            //ランバート
            float cos = saturate(NdotL);
            outputColor.rgb *= gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
        }
        else if (gDirectionalLight.lightingType == 2)
        {
            // ハーフランバート
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            outputColor.rgb *= gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
        }

    }

    return outputColor;
}