#include "Bloom.h"
#include "PostProcessResources.h"
#include <cmath>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif
using namespace PostProcessResources;

void Bloom::Initialize(DirectXCommon* dxCommon, int width, int height) {
    Validate(dxCommon, width, height);
    if (dxCommon_) {
        throw std::logic_error("Bloom is already initialized");
    }

    width_ = width;
    height_ = height;
    dxCommon_ = dxCommon;
    auto* device = dxCommon->GetDevice();
    // 発光対象のHDR画像、横ぼかしの中間画像、最終結果を別々に保持する。
    hdrTexture_ =
        Texture(device, width, height, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
    result_ = Texture(device, width, height, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                      D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    scratch_ = Texture(device, width, height, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
                       D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    rtvHeap_ = Heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, false);
    srvHeap_ = Heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, Count, true);
    device->CreateRenderTargetView(hdrTexture_.Get(), nullptr, rtvHeap_->GetCPUDescriptorHandleForHeapStart());

    // 同じ画像を読み書きの用途に応じてSRV・UAVとして参照する。
    const UINT step = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    auto cpu = [&](UINT index) {
        auto h = srvHeap_->GetCPUDescriptorHandleForHeapStart();
        h.ptr += SIZE_T(step) * index;
        return h;
    };
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
    srv.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Texture2D.MipLevels = 1;
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};
    uav.Format = srv.Format;
    uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateShaderResourceView(hdrTexture_.Get(), &srv, cpu(InputSRV));
    device->CreateUnorderedAccessView(result_.Get(), nullptr, &uav, cpu(ResultUAV));
    device->CreateShaderResourceView(result_.Get(), &srv, cpu(ResultSRV));
    device->CreateUnorderedAccessView(scratch_.Get(), nullptr, &uav, cpu(ScratchUAV));
    device->CreateShaderResourceView(scratch_.Get(), &srv, cpu(ScratchSRV));

    // t0: 元画像、t1: 横ぼかし、u0: 出力画像、b0: ぼかし設定。
    HRESULT hr;
    D3D12_DESCRIPTOR_RANGE descriptorRange[3] = {};

    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].NumDescriptors = 1;
    descriptorRange[0].BaseShaderRegister = 0;
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    descriptorRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descriptorRange[1].NumDescriptors = 1;
    descriptorRange[1].BaseShaderRegister = 0;
    descriptorRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[4] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[0].DescriptorTable.pDescriptorRanges = &descriptorRange[0];
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[1].DescriptorTable.pDescriptorRanges = &descriptorRange[1];
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[2].Constants.ShaderRegister = 0;
    rootParameters[2].Constants.Num32BitValues = 4;
    rootParameters[2].Constants.RegisterSpace = 0;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    descriptorRange[2] = descriptorRange[0];
    descriptorRange[2].BaseShaderRegister = 1;
    rootParameters[3] = rootParameters[0];
    rootParameters[3].DescriptorTable.pDescriptorRanges = &descriptorRange[2];
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.NumParameters = 4;
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signatureBlob, &errorBlob);
    Check(hr);
    hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
                                     IID_PPV_ARGS(&rootSignature_));
    Check(hr);

    auto shader = dxCommon->CompilerShader(L"Resources/Shaders/BloomBlur.CS.hlsl", L"cs_6_0", dxCommon->GetDxcUtils(),
                                           dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler());
    D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline{};
    pipeline.pRootSignature = rootSignature_.Get();
    pipeline.CS = {shader->GetBufferPointer(), shader->GetBufferSize()};
    Check(device->CreateComputePipelineState(&pipeline, IID_PPV_ARGS(&pipeline_)));

    // 発光画像の合成には加算用の全画面描画を使う。
    composite_.Initialize(dxCommon, true);
    compositeSettings_.toneMapping = ToneMapping::ACESApproximation;
}

D3D12_GPU_DESCRIPTOR_HANDLE Bloom::Handle(UINT index) const {
    auto h = srvHeap_->GetGPUDescriptorHandleForHeapStart();
    h.ptr += UINT64(index) *
             dxCommon_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    return h;
}

void Bloom::PreDraw() {
    if (!pipeline_ || drawing_) {
        throw std::logic_error("Invalid Bloom capture begin");
    }

    auto* command = dxCommon_->GetCommandList();
    Transition(command, hdrTexture_.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // 通常描画と深度バッファを共有し、発光物も前後関係を判定できるようにする。
    auto rtv = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    auto dsv = dxCommon_->GetDsvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
    command->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
    D3D12_VIEWPORT viewport{0, 0, float(width_), float(height_), 0, 1};
    D3D12_RECT scissor{0, 0, width_, height_};
    command->RSSetViewports(1, &viewport);
    command->RSSetScissorRects(1, &scissor);
    const float black[4] = {0, 0, 0, 1};
    command->ClearRenderTargetView(rtv, black, 0, nullptr);
    drawing_ = true;
    inputReady_ = false;
    resultReady_ = false;
}

void Bloom::PostDraw() {
    if (!drawing_) {
        throw std::logic_error("Bloom capture was not begun");
    }

    Transition(dxCommon_->GetCommandList(), hdrTexture_.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET,
               D3D12_RESOURCE_STATE_GENERIC_READ);
    dxCommon_->SetBackBufferRenderTarget();
    drawing_ = false;
    inputReady_ = true;
}

void Bloom::Execute() {
    if (!inputReady_ || drawing_) {
        throw std::logic_error("Bloom input is not ready");
    }

    auto* command = dxCommon_->GetCommandList();
    command->SetPipelineState(pipeline_.Get());
    command->SetComputeRootSignature(rootSignature_.Get());
    ID3D12DescriptorHeap* heaps[] = {srvHeap_.Get()};
    command->SetDescriptorHeaps(1, heaps);

    // 横パス: 明るい成分を抽出して中間画像へ書き込む。
    command->SetComputeRootDescriptorTable(0, Handle(InputSRV));
    command->SetComputeRootDescriptorTable(1, Handle(ScratchUAV));
    command->SetComputeRootDescriptorTable(3, Handle(InputSRV));

    struct Constants {
        int vertical;
        float threshold;
        float intensity;
        float padding;
    } constants{0, threshold_, intensity_, 0};

    static_assert(sizeof(Constants) == 16);
    command->SetComputeRoot32BitConstants(2, 4, &constants, 0);
    command->Dispatch((width_ + 7) / 8, (height_ + 7) / 8, 1);

    // 縦パス: 横パスの出力を読み取れる状態へ切り替えてから使う。
    Transition(command, scratch_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
               D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    command->SetComputeRootDescriptorTable(1, Handle(ResultUAV));
    command->SetComputeRootDescriptorTable(3, Handle(ScratchSRV));
    constants.vertical = 1;
    command->SetComputeRoot32BitConstants(2, 4, &constants, 0);
    command->Dispatch((width_ + 7) / 8, (height_ + 7) / 8, 1);
    Transition(command, scratch_.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
               D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    inputReady_ = false;
    resultReady_ = true;
}

void Bloom::DrawResult() {
    if (!resultReady_) {
        throw std::logic_error("Bloom result is not ready");
    }

    auto* command = dxCommon_->GetCommandList();
    dxCommon_->SetBackBufferRenderTarget();

    // 計算結果をPSの入力に切り替え、バックバッファへ加算合成する。
    Transition(command, result_.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
               D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    composite_.Draw(command, srvHeap_.Get(), Handle(ResultSRV), compositeSettings_);
    Transition(command, result_.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
               D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    resultReady_ = false;
}

void Bloom::SetThreshold(float value) {
    if (!std::isfinite(value) || value < 0 || value > 100) {
        throw std::invalid_argument("Invalid Bloom threshold");
    }

    threshold_ = value;
}

void Bloom::SetIntensity(float value) {
    if (!std::isfinite(value) || value < 0 || value > 100) {
        throw std::invalid_argument("Invalid Bloom intensity");
    }

    intensity_ = value;
}

void Bloom::SetCompositeSettings(const PostProcessSettings& settings) {
    settings.Validate();
    compositeSettings_ = settings;
}

void Bloom::DrawImGui() {
#ifdef USE_IMGUI
    if (ImGui::TreeNode("Bloom")) {
        ImGui::SliderFloat("Threshold", &threshold_, 0, 5);
        ImGui::SliderFloat("Glow intensity", &intensity_, 0, 5);
        ImGui::TreePop();
    }
#endif
}
