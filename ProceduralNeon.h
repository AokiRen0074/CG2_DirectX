#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include "Vector2.h"
#include "Vector3.h"
#include "Matrix4x4.h"
#include "ViewProjection.h"
#include "DirectXCommon.h"

// ==========================================
// 責務：プログラムで動的に生成される「単一のネオン管」
// 依存：外部ファイル(OBJ)や複雑なパイプライン管理には一切関与しない
// ==========================================
class ProceduralNeon {
public:
	// C++側の責務：ただの板（4頂点）を作るだけ
	struct VertexData {
		Vector3 pos;
		Vector3 normal; // シェーダーエラー回避用のダミー
		Vector2 uv;
	};

	struct TransformData {
		Matrix4x4 matWorld;
	};

	struct MaterialData {
		float color[4]; // Vector4エラー回避のため配列を使用
		float intensity;
		float radius;
		float softness;
		float tubeLength;
	};

	ProceduralNeon() = default;
	~ProceduralNeon();

	void Initialize(DirectXCommon* dxCommon);

	// 外部から「事実（どこからどこへ引くか、カメラはどこか）」だけを受け取る
	void Update(const Vector3& start, const Vector3& end, float thickness, const Vector3& cameraPos, const ViewProjection& viewProj);

	// 質感（フリッカーや色）を受け取る
	void SetMaterial(float intensity, const Vector3& color, float radius, float softness, float tubeLength);

	void Draw(DirectXCommon* dxCommon);

private:
	// D3D12 リソース（完全に自己完結して管理する）
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vbView_{};
	VertexData* mappedVertex_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> transformBuffer_;
	TransformData* mappedTransform_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> materialBuffer_;
	MaterialData* mappedMaterial_ = nullptr;
};