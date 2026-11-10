#include "ScenePostProcess.h"
#include "PostProcessResources.h"
#include <algorithm>
using namespace PostProcessResources;

void ScenePostProcess::Initialize(DirectXCommon* dxCommon) {
    Validate(dxCommon, 1, 1);
    if (dxCommon_) {
        throw std::logic_error("Scene post-process is already initialized");
    }

    dxCommon_ = dxCommon;
    srvHeap_ = Heap(dxCommon->GetDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, true);
    fullscreen_.Initialize(dxCommon, false);
}

void ScenePostProcess::SetSettings(const PostProcessSettings& settings) {
    settings.Validate();
    settings_ = settings;
}

void ScenePostProcess::SetRadialBlur(float strength, float x, float y) {
    if (!std::isfinite(strength) || !std::isfinite(x) || !std::isfinite(y)) {
        throw std::invalid_argument("Invalid radial blur");
    }

    auto settings = settings_;
    settings.radialBlur = std::clamp(strength, 0.0f, 0.25f);
    settings.centerX = std::clamp(x, 0.0f, 1.0f);
    settings.centerY = std::clamp(y, 0.0f, 1.0f);
    SetSettings(settings);
}

bool ScenePostProcess::Apply() {
    if (!dxCommon_) {
        throw std::logic_error("Scene post-process is not initialized");
    }

    // 加工が無効なら画像コピーと全画面描画を省略する。
    if (settings_.IsIdentity()) {
        return false;
    }
    auto* device = dxCommon_->GetDevice();
    Microsoft::WRL::ComPtr<ID3D12Resource> backBuffer;
    Check(dxCommon_->GetSwapChain()->GetBuffer(dxCommon_->GetSwapChain()->GetCurrentBackBufferIndex(),
                                               IID_PPV_ARGS(&backBuffer)));
    auto desc = backBuffer->GetDesc();

    // 初回だけコピー先を作り、以降のフレームでは再利用する。
    if (!scene_) {
        desc.Flags = D3D12_RESOURCE_FLAG_NONE;
        D3D12_HEAP_PROPERTIES heap{};
        heap.Type = D3D12_HEAP_TYPE_DEFAULT;
        Check(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
                                              D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr,
                                              IID_PPV_ARGS(&scene_)));
        D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
        srv.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv.Texture2D.MipLevels = 1;
        device->CreateShaderResourceView(scene_.Get(), &srv, srvHeap_->GetCPUDescriptorHandleForHeapStart());
    } else if (scene_->GetDesc().Width != desc.Width || scene_->GetDesc().Height != desc.Height ||
               scene_->GetDesc().Format != desc.Format) {
        throw std::logic_error("Recreate ScenePostProcess after a swap-chain resize");
    }

    // 入力と描画先が同じ画像にならないよう、バックバッファを退避する。
    auto* command = dxCommon_->GetCommandList();
    Transition(command, backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    Transition(command, scene_.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    command->CopyResource(scene_.Get(), backBuffer.Get());
    Transition(command, backBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    Transition(command, scene_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // コピーした画像をPSで加工して、元の描画先へ戻す。
    dxCommon_->SetBackBufferRenderTarget();
    D3D12_VIEWPORT viewport{0, 0, float(desc.Width), float(desc.Height), 0, 1};
    D3D12_RECT scissor{0, 0, LONG(desc.Width), LONG(desc.Height)};
    command->RSSetViewports(1, &viewport);
    command->RSSetScissorRects(1, &scissor);
    fullscreen_.Draw(command, srvHeap_.Get(), srvHeap_->GetGPUDescriptorHandleForHeapStart(), settings_);
    return true;
}
