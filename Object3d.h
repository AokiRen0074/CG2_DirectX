#pragma once
#include "DirectXCommon.h"
#include <wrl.h>
#include "Logger.h"
#include <string>
#include "Matrix4x4.h"

class Object3d {

public:


	struct Transform {
		Vector3  scale;
		Vector3 rotate;
		Vector3 translate;
	};



	void Initialize(DirectXCommon* dxCommon);

	void Update();


	void Draw();

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
};