struct TransformationMatrix
{
	float4x4 WVP;
	float4x4 World;
};

// C++の rootParameters[1] から受け取るデータ
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);

// C++から送られてくる頂点データ
struct VSInput
{
	float4 pos : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
};

// PS（ピクセルシェーダー）へ送るデータ
struct VSOutput
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL; 
	float3 posWorld : TEXCOORD1; 
};

VSOutput main(VSInput input)
{
	VSOutput output;
    
    // 座標とUVはそのまま計算
	output.pos = mul(input.pos, gTransformationMatrix.WVP);
	output.uv = input.uv;
    
    // ✨ここが最大の鍵！
    // モデルが回転したら、法線（面の向き）も一緒に回転させてからPSに渡す！
	output.normal = normalize(mul(input.normal, (float3x3) gTransformationMatrix.World));
    
	return output;
}