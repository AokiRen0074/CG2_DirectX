cbuffer Camera : register(b0) { row_major float4x4 viewProjection; };
struct Vertex {
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    float shape : TEXCOORD1;
};
struct Output {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    float4 color : COLOR0;
    nointerpolation float shape : TEXCOORD1;
};
Output main(Vertex input) {
    Output output;
    output.position = mul(float4(input.position, 1), viewProjection);
    output.uv = input.uv;
    output.color = input.color;
    output.shape = input.shape;
    return output;
}
