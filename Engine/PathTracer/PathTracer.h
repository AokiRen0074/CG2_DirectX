#pragma once
#include "Base/DirectXCommon.h"
#include "Vector4.h"
#include <wrl.h>

class PathTracer {
public:
	// 頂点データ
	struct VertexData {
		Vector4 position;
		float texcoord[2];
	};

	// シェーダーに送る時間と解像度データ
	struct SceneData {
		float time;
		float resolution[2];
		float frameCount;
	};

	static void StaticInitialize(DirectXCommon* dxCommon);

	void Initialize();
	void Update();
	void Draw();

private:
	static DirectXCommon* sDxCommon_;
	DirectXCommon* dxCommon_ = nullptr;

	// パイプライン関連
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;

	// 頂点・インデックスバッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
	uint32_t indexCount_ = 0;

	// シーンデータ
	Microsoft::WRL::ComPtr<ID3D12Resource> sceneDataResource_;
	SceneData* sceneDataMap_ = nullptr;

	float currentTime_ = 0.0f;

	uint32_t frameCount_ = 0; // 現在の蓄積フレーム数
	Microsoft::WRL::ComPtr<ID3D12Resource> accumulationTexture_; // 見えないキャンバス
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> uavHeap_;       // キャンバスへのアクセス権

	// バッファ作成用
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);

	// キャンバス作成用
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateUAVTextureResource(ID3D12Device* device, uint32_t width, uint32_t height);
};