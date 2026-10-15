#pragma once
#include <d3d12.h>
#include <d3dcompiler.h> // ★追加：シェーダー読み込み用
#include <wrl.h>
#pragma comment(lib, "d3dcompiler.lib")
#include "DirectXCommon.h"

class Bloom {
public:
	// 初期化（画面サイズを受け取る）
	void Initialize(DirectXCommon* dxCommon, int windowWidth, int windowHeight);

	// 描画前の準備（HDRキャンバスをセット）
	void PreDraw();
	// 描画後の処理（HDRキャンバスを画像として保存）
	void PostDraw();

	void Execute();

	// ★追加：計算結果を画面に描画する
	void DrawResult();

	// 通常の画面を読み込み、UIの前に撃破時の放射状ブラーをかける。
	void CaptureScene();
	void SetRadialBlur(float strength, float centerX, float centerY);

	// 光だけを合成するモード。色収差・ノイズを切り、分離ガウスぼかしを使う。
    void EnableEmissionLayer();
    void DrawImGui();

private:
	DirectXCommon* dxCommon_ = nullptr;

	bool enableLuminance_ = true;
	bool enableBlur_ = true;
	bool enableAdditive_ = true;
	bool useACES_ = true;

	float time_ = 0.0f;                  // ノイズを動かすための時間
	float chromaticAberration_ = 0.005f; // 色収差の強さ
	float noiseIntensity_ = 0.05f;       // ノイズの強

	// HDR描画用のテクスチャリソース（1.0以上の色を保存できるキャンバス）
	Microsoft::WRL::ComPtr<ID3D12Resource> hdrTextureResource_;

	// デバイス取得用のヘルパー（KamataEngineの機能を利用）
	ID3D12Device* GetDevice();

	// RTV（レンダーターゲットビュー）用デスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle_{};

	// SRV（シェーダーリソースビュー）用デスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle_{};

	// コマンドリスト取得用のヘルパー
	ID3D12GraphicsCommandList* GetCommandList();

	// CSの処理結果を書き込むためのキャンバス（UAV）
	Microsoft::WRL::ComPtr<ID3D12Resource> uavTextureResource_;

	// ルートシグネチャ（シェーダーとの橋渡し役）
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;

	// パイプラインステート（コンピュートシェーダーの実行状態）
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

	// 画面描画（ポストプロセス）用の契約書と実行状態
	Microsoft::WRL::ComPtr<ID3D12RootSignature> postProcessRootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> postProcessPipelineState_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> scenePostPipelineState_;
	Microsoft::WRL::ComPtr<ID3D12Resource> sceneTextureResource_;
	bool sceneMode_ = false;
    bool emissionLayer_ = false;
    int width_ = 1280, height_ = 720;
    Microsoft::WRL::ComPtr<ID3D12Resource> blurScratch_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> emissionBlurPipeline_;
    void ExecuteEmissionBlur();
	float radialBlur_ = 0.0f;
	float blurCenterX_ = 0.5f;
	float blurCenterY_ = 0.5f;
};
