// NeonModel.PS.hlsl

// C++側の ColorData と完全に一致させる定数バッファ
cbuffer NeonSettings : register(b1)
{
    float4 neonColor;
    float intensity;
    float radius; // 3Dモデルでは使わないですが、C++側の構造体に合わせるために残します
    float softness; // 同上
    float length; // 同上
};

// 頂点シェーダーから渡ってくるデータ（Object3d系のVSと合わせます）
struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

float4 main(VSOutput input) : SV_TARGET
{
    // ==========================================
    // 3Dモデル専用のネオン発光計算
    // ==========================================
    
    // ネオンの色に強度(intensity)を掛けて、1.0以上のHDRカラー（爆光）を作る！
    // Bloomシェーダーがこの「1.0を超えた光」を検知してボワっと光らせてくれます。
    float3 hdrColor = neonColor.rgb * intensity;

    return float4(hdrColor, 1.0f);
}