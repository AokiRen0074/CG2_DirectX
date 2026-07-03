struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

// 頂点バッファ(C++からのデータ)は受け取らず、頂点番号(VertexID)だけを受け取る
VSOutput main(uint vertexID : SV_VertexID)
{
    VSOutput output;
    
    // 頂点番号(0, 1, 2)から、画面を覆う巨大な三角形のUV座標(0,0 ～ 2,2)を計算する小技
    output.uv = float2((vertexID << 1) & 2, vertexID & 2);
    
    // UV座標から画面上の位置(X, Y)を計算
    output.pos = float4(output.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    
    return output;
}