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
    float32_t4 position : SV_POSITION;
    float32_t2 texcoord : TEXCOORD0;
    float32_t3 normal : NORMAL0;
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
    output.normal = normalize(mul(input.normal, (float32_t3x3) gTransformationMatrix.World));
    
    return output;
}