#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "Vector4.h"
#include "Vector3.h"
#include "Vector2.h"
#include "Matrix4x4.h"
#include "DirectXCommon.h"


struct ConstBufferDataPlayer {
	 Vector4 neonColor;
	float intensity;
	float radius;
	float softness;
	float tubeLength;
};

class NeonPlayer {
public:
	void Initialize(DirectXCommon* dxCommon);
	void Update(float shakeX, float shakeY);
	void Draw();
	void DrawImGui();

	// 引数を Vector3に統一し、使わないrotを削除
	void SetTransform( Vector3 pos,  Vector3 scale) {
		position_ = pos;
		scale_ = scale;
	}

	// position_ も Vector3 になったので、これで完璧にエラーが消えます！
	 Vector3 GetPosition() const { return position_; }

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vbView_{};
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;
	D3D12_INDEX_BUFFER_VIEW ibView_{};

	Microsoft::WRL::ComPtr<ID3D12Resource> cbTransform_;
	Microsoft::WRL::ComPtr<ID3D12Resource> cbColor_;

	DirectXCommon* dxCommon_ = nullptr;

	struct Vertex {
		 Vector3 pos; // ここもVector3に統一
		 Vector2 uv;
	};

	struct TransformData {
		 Matrix4x4 WVP;
	};

	struct ColorData {
		 Vector4 color; // XMFLOAT4から変更
		float intensity;
		float radius;
		float softness;
		float length;
	};

	TransformData* mappedTransform_ = nullptr;
	ColorData* mappedColor_ = nullptr;

	// トランスフォーム保持用（XMFLOAT3からVector3に変更）
	 Vector3 position_ = {0.0f, 0.0f, 0.0f};
	 Vector3 scale_ = {1.0f, 1.0f, 1.0f};
	float rotationZ_ = 0.0f;

	float tubeLength_ = 1.0f;

	float playerColor_[3] = {0.0f, 1.0f, 0.3f};
	float playerIntensity_ = 8.0f;
	float playerRadius_ = 0.02f;
};