#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <cstdint>
#include <dxcapi.h>
#include <string>

#pragma comment(lib,"dxcompiler.lib")

class WindowApp;

class DirectXCommon {
public:
    // 初期化
    void Initialize(WindowApp* winApp);

    // 描画処理
    void PreDraw();
	void PostDraw();

    // コマンドを実行してGPUを待つ関数
    void FlushCommandList();

    // ゲッター
    ID3D12Device* GetDevice() const { return device_.Get(); }
    ID3D12CommandQueue* GetCommandQueue() const { return commandQueue_.Get(); }
    ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }
    IDXGISwapChain4* GetSwapChain() const { return swapChain_.Get(); }
    ID3D12DescriptorHeap* GetSrvDescriptorHeap() const { return srvDescriptorHeap_.Get(); }

    IDxcUtils* GetDxcUtils() const { return dxcUtils_.Get(); }
    IDxcCompiler3* GetDxcCompiler() const { return dxcCompiler_.Get(); }
    IDxcIncludeHandler* GetIncludeHandler() const { return includeHandler_.Get(); }

    // CompileShader関数
    Microsoft::WRL::ComPtr<IDxcBlob>CompilerShader(
        const std::wstring& filePath,
        const wchar_t* profile,
        IDxcUtils* dxcUtils,
        IDxcCompiler3* dxcCompiler,
        IDxcIncludeHandler* includeHandler
    );

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

    // FenceとEvent
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
	uint64_t fenceValue_ = 0;
	HANDLE fenceEvent_ = nullptr;

    // DXCの初期化
    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler_;

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(
        ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

    // imguiでつかうSRV用のヒープ
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap_;


};