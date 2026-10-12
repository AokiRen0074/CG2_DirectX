Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(VSOutput input) : SV_TARGET
{
    // ‰æ‘œ‚ÌF‚ğ‚»‚Ì‚Ü‚Üo—Í‚·‚é
    return gTexture.Sample(gSampler, input.uv);
}