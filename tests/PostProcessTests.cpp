#include "Base/DirectXCommon.h"
#include "Base/WindowApp.h"
#include "Base/Logger.h"
#include "PostProcess/Bloom.h"
#include "PostProcess/ScenePostProcess.h"
#include "PostProcess/PostProcessResources.h"
#include <d3d12sdklayers.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <limits>
#include <cstring>
using Microsoft::WRL::ComPtr;
using namespace PostProcessResources;

void Require(bool value, const char* why) {
    if (!value) {
        throw std::runtime_error(why);
    }
}

// GPU完了を待ってから、実際の描画結果の中央画素を読み戻す。
float ReadRed(DirectXCommon& dx) {
    ComPtr<ID3D12Resource> back;
    Check(dx.GetSwapChain()->GetBuffer(dx.GetSwapChain()->GetCurrentBackBufferIndex(), IID_PPV_ARGS(&back)));
    auto desc = back->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    UINT64 bytes = 0;
    dx.GetDevice()->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, nullptr, nullptr, &bytes);
    D3D12_HEAP_PROPERTIES heap{};
    heap.Type = D3D12_HEAP_TYPE_READBACK;
    D3D12_RESOURCE_DESC buffer{};
    buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer.Width = bytes;
    buffer.Height = 1;
    buffer.DepthOrArraySize = 1;
    buffer.MipLevels = 1;
    buffer.SampleDesc.Count = 1;
    buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> readback;
    Check(dx.GetDevice()->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer, D3D12_RESOURCE_STATE_COPY_DEST,
                                                  nullptr, IID_PPV_ARGS(&readback)));
    auto* command = dx.GetCommandList();
    Transition(command, back.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
    D3D12_TEXTURE_COPY_LOCATION source{};
    source.pResource = back.Get();
    source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    D3D12_TEXTURE_COPY_LOCATION dest{};
    dest.pResource = readback.Get();
    dest.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dest.PlacedFootprint = footprint;
    command->CopyTextureRegion(&dest, 0, 0, 0, &source, nullptr);
    Transition(command, back.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    dx.FlushCommandList();
    void* data = nullptr;
    Check(readback->Map(0, nullptr, &data));
    const auto offset =
        footprint.Offset + UINT64(desc.Height / 2) * footprint.Footprint.RowPitch + (desc.Width / 2) * 4;
    const float result = static_cast<unsigned char*>(data)[offset] / 255.0f;
    readback->Unmap(0, nullptr);
    return result;
}

int main() {
    bool com = false, window = false;
    try {
        Check(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
        com = true;
        ComPtr<ID3D12Debug> debug;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
            debug->EnableDebugLayer();
        }
        Logger::Initialize();
        auto* win = WindowApp::GetInstance();
        win->Initialize();
        window = true;
        ShowWindow(win->GetHwnd(), SW_HIDE);
        {
            DirectXCommon dx;
            dx.Initialize(win);
            Bloom bloom;
            bloom.Initialize(&dx, 1280, 720);
            ScenePostProcess scene;
            scene.Initialize(&dx);
            PostProcessSettings settings;
            Require(settings.IsIdentity(), "Default settings must skip scene processing");
            auto invalid = settings;
            invalid.radialBlur = std::numeric_limits<float>::quiet_NaN();
            bool rejected = false;
            try {
                scene.SetSettings(invalid);
            } catch (const std::invalid_argument&) {
                rejected = true;
            }
            Require(rejected, "NaN settings were accepted");
            rejected = false;
            try {
                bloom.Execute();
            } catch (const std::logic_error&) {
                rejected = true;
            }
            Require(rejected, "Bloom accepted missing capture");
            // 計算結果を比較できるよう、赤の線形値が0.4の一様なHDR画像を描く。
            auto vs = dx.CompilerShader(L"Resources/Shaders/PostProcess.VS.hlsl", L"vs_6_0", dx.GetDxcUtils(),
                                        dx.GetDxcCompiler(), dx.GetIncludeHandler());
            const char text[] = "float4 main() : SV_TARGET { return float4(0.4,0,0,1); }";
            DxcBuffer input{text, sizeof(text) - 1, DXC_CP_UTF8};
            const wchar_t* args[] = {L"-E", L"main", L"-T", L"ps_6_0"};
            ComPtr<IDxcResult> compiled;
            Check(dx.GetDxcCompiler()->Compile(&input, args, 4, dx.GetIncludeHandler(), IID_PPV_ARGS(&compiled)));
            HRESULT status;
            Check(compiled->GetStatus(&status));
            Check(status);
            ComPtr<IDxcBlob> ps;
            Check(compiled->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&ps), nullptr));
            D3D12_ROOT_SIGNATURE_DESC signature{};
            ComPtr<ID3DBlob> blob, errors;
            Check(D3D12SerializeRootSignature(&signature, D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, &errors));
            ComPtr<ID3D12RootSignature> root;
            Check(dx.GetDevice()->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(),
                                                      IID_PPV_ARGS(&root)));
            D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
            desc.pRootSignature = root.Get();
            desc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
            desc.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
            desc.SampleMask = UINT_MAX;
            desc.SampleDesc.Count = 1;
            desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
            desc.RasterizerState.DepthClipEnable = TRUE;
            desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            desc.NumRenderTargets = 1;
            desc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
            desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
            ComPtr<ID3D12PipelineState> emitter;
            Check(dx.GetDevice()->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&emitter)));
            bloom.SetCompositeSettings(settings);
            bloom.SetIntensity(1);
            const auto render = [&](float threshold) {
                dx.PreDraw();
                Require(!scene.Apply(), "Identity scene processing was not skipped");
                bloom.SetThreshold(threshold);
                bloom.PreDraw();
                auto* cmd = dx.GetCommandList();
                cmd->SetGraphicsRootSignature(root.Get());
                cmd->SetPipelineState(emitter.Get());
                cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                cmd->DrawInstanced(3, 1, 0, 0);
                bloom.PostDraw();
                bloom.Execute();
                bloom.DrawResult();
                const float red = ReadRed(dx);
                dx.PostDraw();
                return red;
            };
            const float noGlow = render(0.75f), withGlow = render(0.2f);
            const auto srgb = [](float linear) {
                return linear <= .0031308f ? linear * 12.92f : 1.055f * std::pow(linear, 1 / 2.4f) - .055f;
            };
            Require(std::abs(noGlow - srgb(.5f)) < .02f, "Bloom base emission changed unexpectedly");
            Require(std::abs(withGlow - srgb(.7f)) < .02f,
                    "Separable blur/composite differs from uniform-image reference");
            // 画面のコピー・PS加工・次フレームでのリソース再利用を確認する。
            scene.SetRadialBlur(.1f, .5f, .5f);
            for (int i = 0; i < 2; ++i) {
                dx.PreDraw();
                Require(scene.Apply(), "Enabled scene processing was skipped");
                Require(std::abs(ReadRed(dx) - srgb(.1f)) < .02f, "Scene copy or radial blur changed a uniform image");
                dx.PostDraw();
            }
            ComPtr<ID3D12InfoQueue> queue;
            if (SUCCEEDED(dx.GetDevice()->QueryInterface(IID_PPV_ARGS(&queue)))) {
                for (UINT64 i = 0; i < queue->GetNumStoredMessages(); ++i) {
                    SIZE_T size = 0;
                    Check(queue->GetMessage(i, nullptr, &size));
                    std::vector<unsigned char> data(size);
                    auto* message = reinterpret_cast<D3D12_MESSAGE*>(data.data());
                    Check(queue->GetMessage(i, message, &size));
                    if (message->Severity <= D3D12_MESSAGE_SEVERITY_ERROR) {
                        throw std::runtime_error(message->pDescription);
                    }
                }
            }
            std::cout << "PASS: GPU Bloom readback, threshold, scene pass reuse, validation and D3D12 states.\n";
        }
        win->Finalize();
        window = false;
        Logger::Finalize();
        CoUninitialize();
        com = false;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        if (window) {
            WindowApp::GetInstance()->Finalize();
        }
        Logger::Finalize();
        if (com) {
            CoUninitialize();
        }
        return 1;
    }
}
