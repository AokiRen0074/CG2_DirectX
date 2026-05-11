#pragma once
#include "DirectXCommon.h"
#include <wrl.h>
#include "Logger.h"
#include <string>
#include "Matrix4x4.h"

class Object3d {

public:

	struct Vector4 {
		float x, y, z, w;
	};

	struct Vector2 {
		float x, y;
	};

	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
	};

	struct Transform {
		Vector3  scale;
		Vector3 rotate;
		Vector3 translate;
	};



	void Initialize(DirectXCommon* dxCommon);

	void Update();


	void Draw();

	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSrvHandleGPU() const { return textureSrvHandleGPU_; }

	// ゲッター
	ID3D12RootSignature* GetRootSignature() { return rootSignature_.Get(); }
	ID3D12PipelineState* GetGraphicsPipelineState()const { return graphicsPipelineState_.Get(); }

private:
	DirectXCommon* dxCommon_ = nullptr;

	// rootSignatureとGraphicPipelineState
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;

	// 頂点データ
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResources_;

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);

	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;

	// 自分の位置　回転スケールを持つ変数
	Transform transform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	Transform cameraTransform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -5.0f} };

	Matrix4x4* wvpData_ = nullptr;

	// テクスチャ追加用のリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;

	// テクスチャのGPU上のアドレス
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_;

	// マテリアル用の色
	Vector4* materialData_ = nullptr;
	Vector4 materialColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };
};