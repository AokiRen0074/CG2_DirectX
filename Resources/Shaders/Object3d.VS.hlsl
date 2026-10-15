// ==========================================
// 構造体の定義
// ==========================================

// 座標変換行列
struct TransformationMatrix
{
    float32_t4x4 WVP;
    float32_t4x4 World;
};

// C++から受け取る入力データ
struct VertexShaderInput
{
    float32_t4 position : POSITION0;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
};

// ピクセルシェーダーへ送る出力データ
struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    
    //  唯一の追加点：ワールド座標をPSに投げるためのセマンティクス
    float3 worldPos : TEXCOORD1;
};

// 定数バッファ
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

// ==========================================
// メイン関数
// ==========================================
VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    
    // 頂点の位置を変換
    output.position = mul(input.position, gTransformationMatrix.WVP);
    
    // UV座標はそのまま渡す
    output.texcoord = input.texcoord;
    
    // 法線をワールド座標系に変換し、正規化する
    float3x3 world = (float3x3)gTransformationMatrix.World;
    float3x3 normalMatrix = float3x3(cross(world[1], world[2]), cross(world[2], world[0]), cross(world[0], world[1]));
    output.normal = normalize(mul(input.normal, normalMatrix));
    
    //  これが原因のエラーを消すための1行
    output.worldPos = mul(input.position, gTransformationMatrix.World).xyz;
    
    return output;
}