#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

class WindowApp;

class DirectXCommon {
public:
    // 初期化
    void Initialize(WindowApp* winApp);

    // 描画処理
    void PreDraw();
	void PostDraw();

    // ゲッター
    ID3D12Device* GetDevice() const { return device_.Get(); }
    ID3D12CommandQueue* GetCommandQueue() const { return commandQueue_.Get(); }
    ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }
    IDXGISwapChain4* GetSwapChain() const { return swapChain_.Get(); }


private:
    Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory_;
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator_;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;

    // スワップチェーンを生成する
    Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain_;

	// ディスクリプタヒープを生成する
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;

    // スワップチェーンからリソースを持ってくる
	Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources_[2];

    // RTVを2つ作るのでディスクリプタを2つ取得
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles_[2];

    //エラー、警告を出す
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue_;

};