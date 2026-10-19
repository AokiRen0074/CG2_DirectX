struct VertexData
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

VertexShaderOutput main(VertexData input)
{
    VertexShaderOutput output;
    output.position = input.position;
    output.texcoord = input.texcoord;
    output.color = input.color;
    return output;
}