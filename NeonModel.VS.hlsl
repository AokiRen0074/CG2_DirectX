// ==========================================
// 構造体の定義
// ==========================================

struct TransformationMatrix
{
    float4x4 WVP;
    float4x4 World;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPos : TEXCOORD1; // 🌟 修正：PS側と合わせるために TEXCOORD1 にする
};

ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

// ==========================================
// メイン関数 (エントリーポイント)
// ==========================================
VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    // 頂点の位置を変換
    output.position = mul(input.position, gTransformationMatrix.WVP);
    
    // UV座標はそのまま渡す
    output.texcoord = input.texcoord;
    
    // 法線をワールド座標系に変換し、正規化する
    output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.World));
    
    // ワールド座標を計算してPSへ送る
    output.worldPos = mul(input.position, gTransformationMatrix.World).xyz;
    
    return output;
}