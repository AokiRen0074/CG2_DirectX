#pragma once
#include "DirectXCommon.h"
#include <wrl.h>
#include "Logger.h"
#include <string>

class Object3d {

public:
	void Initialize(DirectXCommon* dxCommon);

	void Draw();

	// ゲッター
	ID3D12RootSignature* GetRootSignature() { return rootSignature_.Get(); }
	ID3D12PipelineState* GetGraphicsPipelineState()const { return graphicsPipelineState_.Get(); }

private:
	DirectXCommon* dxCommon_ = nullptr;

	// rootSignatureとGraphicPiplineState
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;

	// 頂点データ
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResources_;

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);

};