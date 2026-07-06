#pragma once
#include <DirectXMath.h>
#include <d3d12.h>
#include <wrl.h>
#include "DirectXCommon.h"
#include "Matrix4x4.h"

struct ConstBufferDataNeon {
	DirectX::XMFLOAT4 neonColor; // float4
	float intensity;             // float
	float radius;                // float
	float softness;              // float
	float tubeLength;           
};


class NeonSign {
public:
	void Initialize(DirectXCommon* dxCommon);
	void Update();
	void Draw();

	void UpdateCamera(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);

	// ステップ2で使うための設定関数
	void SetTransform(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale, float rotZ);

void SetTubeLength(float length) { tubeLength_ = length; }

void SetMaterial(float radius, float softness, float intensity, float r, float g, float b, float lengthOffset);



private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vbView_{};
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;
	D3D12_INDEX_BUFFER_VIEW ibView_{};

	Microsoft::WRL::ComPtr<ID3D12Resource> cbTransform_;
	Microsoft::WRL::ComPtr<ID3D12Resource> cbColor_;

	struct Vertex {
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 uv; // シェーダーで管を描くための座標
	};

	struct TransformData {
		DirectX::XMMATRIX WVP;
	};
	struct ColorData {
		DirectX::XMFLOAT4 color;
		float intensity;
		float radius;
		float softness;
		float length;
	};

	TransformData* mappedTransform_ = nullptr;
	ColorData* mappedColor_ = nullptr;

	// トランスフォーム保持用
	DirectX::XMFLOAT3 position_ = {0, 0, 0};
	DirectX::XMFLOAT3 scale_ = {1, 1, 1};
	float rotationY_ = 0.0f;
	float rotationZ_ = 0.0f;

	float tubeLength_ = 1.0f;
	DirectXCommon* dxCommon_ = nullptr;

	float baseLength_ = 0.0f;
};