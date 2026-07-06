#pragma once
#include <DirectXMath.h>
#include "externals/DirectXTex/DirectXTex.h"
#include <d3d12.h>
#include <wrl.h>
#include <string>
#include "DirectXCommon.h"

class NeonImage {
public:
	// 画像のファイルパスを渡して初期化
	void Initialize(DirectXCommon* dxCommon,const wchar_t* filePath);
	void Update();

	// 通常の画像として描画する
	void Draw();
	// 明るい部分だけを抽出してネオンとして描画する
	void DrawLuminance();

	// 発光設定を変える設定の関数
	void SetLuminanceSettings(float threshold, float intensity);

void SetTransform(DirectX::XMFLOAT3 pos, float baseSize, float rotZ);

void DrawImGui(const std::string& label);

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineStateNormal_;    // 通常描画用
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineStateLuminance_; // 輝度抽出用

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vbView_{};

	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;
	D3D12_INDEX_BUFFER_VIEW ibView_{};

	// 画像用
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;

	// 定数バッファ
	Microsoft::WRL::ComPtr<ID3D12Resource> cbTransform_;
	Microsoft::WRL::ComPtr<ID3D12Resource> cbLuminance_;

	struct TransformData {
		DirectX::XMMATRIX WVP;
	};

	struct LuminanceSettings {
		float threshold;
		float intensity;
		float padding[2]; // 16バイト境界のための詰め物
	};

	struct Vertex {
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT2 uv;
	};

	TransformData* mappedTransform_ = nullptr;
	LuminanceSettings* mappedLuminance_ = nullptr;

	DirectX::XMFLOAT3 position_ = {0, 0, 0};
	DirectX::XMFLOAT3 scale_ = {1, 1, 1};
	float rotationZ_ = 0.0f;

	float aspectRatio_ = 1.0f;

	float luminanceThreshold_ = 0.5f; 
	float luminanceIntensity_ = 8.0f;

	DirectXCommon* dxCommon_ = nullptr;
};