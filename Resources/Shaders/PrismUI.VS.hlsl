struct Vertex { float2 position : POSITION; float2 uv : TEXCOORD0; float4 color : COLOR0; };
struct Output { float4 position : SV_POSITION; float2 uv : TEXCOORD0; float4 color : COLOR0; };
Output main(Vertex input) {
    Output o;
    o.position = float4(input.position.x / 640 - 1, 1 - input.position.y / 360, 0, 1);
    o.uv = input.uv; o.color = input.color;
    return o;
}
