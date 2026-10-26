Texture2D atlas : register(t0);
SamplerState linearClamp : register(s0);
struct Input { float4 position : SV_POSITION; float2 uv : TEXCOORD0; float4 color : COLOR0; };
float4 main(Input input) : SV_TARGET {
    return atlas.Sample(linearClamp,input.uv) * input.color;
}
