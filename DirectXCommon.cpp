#include "DirectXCommon.h"
#include "Logger.h"
#include <cassert>
#include <format>
#include "WindowApp.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

/*-----------------------------
初期化
-----------------------------*/
void DirectXCommon::Initialize(WindowApp* winApp) {
    HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory_));
    assert(SUCCEEDED(hr));

    Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;

    for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; i++) {
        DXGI_ADAPTER_DESC3 adapterDesc{};
        hr = useAdapter->GetDesc3(&adapterDesc);
        assert(SUCCEEDED(hr));

        if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
            Logger::Log(std::format("Use Adapter:{}\n", Logger::ConvertString(adapterDesc.Description)));
            break;
        }
        useAdapter.Reset();
    }
    assert(useAdapter != nullptr);

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
    };
    const char* featureLevelStrings[] = { "12.2", "12.1", "12.0" };

    for (size_t i = 0; i < _countof(featureLevels); i++) {
        hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
        if (SUCCEEDED(hr)) {
            Logger::Log(std::format("Feature Level {} is supported.\n", featureLevelStrings[i]));
            break;
        }
    }
    assert(device_ != nullptr);
    Logger::Log("Complete create D3D12 Device.\n");

    // コマンドキューを生成する
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    hr = device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue_));
    assert(SUCCEEDED(hr));

    // コマンドアロケータを生成する
    hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
	assert(SUCCEEDED(hr));

    // コマンドリストを生成する
    hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr, IID_PPV_ARGS(&commandList_));
	assert(SUCCEEDED(hr));  

        // スワップチェーンを生成する
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = WindowApp::kClientWidth;
    swapChainDesc.Height = WindowApp::kClientHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    hr = dxgiFactory_->CreateSwapChainForHwnd(
        commandQueue_.Get(),
        winApp->GetHwnd(),
        &swapChainDesc,
        nullptr,
        nullptr,
        reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf())
    );
    assert(SUCCEEDED(hr));

    // ディスクリプタヒープの生成
	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc_{};
    rtvDescriptorHeapDesc_.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;// レンだーげっとビュー用
	rtvDescriptorHeapDesc_.NumDescriptors = 2;// 裏表で2つ
	hr = device_->CreateDescriptorHeap(&rtvDescriptorHeapDesc_, IID_PPV_ARGS(&rtvDescriptorHeap_));

    assert(SUCCEEDED(hr));

    //スワップチェーンからリソースを持ってくる
    hr = swapChain_->GetBuffer(0, IID_PPV_ARGS(&swapChainResources_[0]));
    assert(SUCCEEDED(hr));
	hr = swapChain_->GetBuffer(1, IID_PPV_ARGS(&swapChainResources_[1]));
	assert(SUCCEEDED(hr));

    // RTVの設定
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;//2Dテクスチャとして読み込む

    // ディスクリプタの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

	// 一つ目のRTV
    rtvHandles_[0] = rtvStartHandle;
    device_->CreateRenderTargetView(swapChainResources_[0].Get(), &rtvDesc, rtvHandles_[0]);

	// 二つ目のRTV(自力)
	rtvHandles_[1].ptr = rtvHandles_[0].ptr + device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	device_->CreateRenderTargetView(swapChainResources_[1].Get(), &rtvDesc, rtvHandles_[1]);

}

/*-------------------------
描画処理
----------------------*/
// 描画前処理
void DirectXCommon::PreDraw() {
    // これから書き込むバックバッファのインデックスを取得
    UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();

    // 描画先のRTVを設定する
    commandList_->OMSetRenderTargets(1, &rtvHandles_[backBufferIndex], false, nullptr);

    // 指定した色で画面全体をクリアする
    float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
    commandList_->ClearRenderTargetView(rtvHandles_[backBufferIndex], clearColor, 0, nullptr);
}

// 描画後処理
void DirectXCommon::PostDraw() {
    // コマンドリストの内容を確定させる
    HRESULT hr = commandList_->Close();
    assert(SUCCEEDED(hr));

    // GPUにコマンドリストの実行を行わせる
    ID3D12CommandList* commandLists[] = { commandList_.Get() };
    commandQueue_->ExecuteCommandLists(1, commandLists);

    // GPUとOSに画面の交換を行うよう通知する
    swapChain_->Present(1, 0);

    // 次のフレーム用のコマンドリストを準備
    hr = commandAllocator_->Reset();
    assert(SUCCEEDED(hr));
    hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
    assert(SUCCEEDED(hr));
}