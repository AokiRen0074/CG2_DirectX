#include "FullscreenPass.h"
#include "PostProcessResources.h"
using namespace PostProcessResources;

void FullscreenPass::Initialize(DirectXCommon* dxCommon_, bool additive) {
    Validate(dxCommon_, 1, 1);
    auto* device = dxCommon_->GetDevice();

    // 入力画像と、PSへ渡す8個の定数をルートシグネチャに定義する。
    HRESULT hr;
    D3D12_DESCRIPTOR_RANGE ppRange{};
    ppRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    ppRange.NumDescriptors = 1;
    ppRange.BaseShaderRegister = 0;
    ppRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER ppParamSRV{};
    ppParamSRV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    ppParamSRV.DescriptorTable.NumDescriptorRanges = 1;
    ppParamSRV.DescriptorTable.pDescriptorRanges = &ppRange;
    ppParamSRV.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_PARAMETER ppParamConstants{};
    ppParamConstants.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    ppParamConstants.Constants.ShaderRegister = 0;
    ppParamConstants.Constants.Num32BitValues = 8;
    ppParamConstants.Constants.RegisterSpace = 0;
    ppParamConstants.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_PARAMETER ppRootParams[2] = {ppParamSRV, ppParamConstants};

    // 画面端では端の画素を参照し、反対側の画像が混ざるのを防ぐ。
    D3D12_STATIC_SAMPLER_DESC samplerDesc{};
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.ShaderRegister = 0;
    samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_ROOT_SIGNATURE_DESC ppRootDesc{};
    ppRootDesc.NumParameters = 2;
    ppRootDesc.pParameters = ppRootParams;
    ppRootDesc.NumStaticSamplers = 1;
    ppRootDesc.pStaticSamplers = &samplerDesc;
    ppRootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    Microsoft::WRL::ComPtr<ID3DBlob> ppSigBlob, ppErrBlob;
    hr = D3D12SerializeRootSignature(&ppRootDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &ppSigBlob, &ppErrBlob);
    Check(hr);
    hr = device->CreateRootSignature(0, ppSigBlob->GetBufferPointer(), ppSigBlob->GetBufferSize(),
                                     IID_PPV_ARGS(&postProcessRootSignature_));
    Check(hr);

    Microsoft::WRL::ComPtr<IDxcBlob> vsBlob =
        dxCommon_->CompilerShader(L"Resources/Shaders/PostProcess.VS.hlsl", L"vs_6_0", dxCommon_->GetDxcUtils(),
                                  dxCommon_->GetDxcCompiler(), dxCommon_->GetIncludeHandler());

    Microsoft::WRL::ComPtr<IDxcBlob> psBlob =
        dxCommon_->CompilerShader(L"Resources/Shaders/PostProcess.PS.hlsl", L"ps_6_0", dxCommon_->GetDxcUtils(),
                                  dxCommon_->GetDxcCompiler(), dxCommon_->GetIncludeHandler());

    D3D12_GRAPHICS_PIPELINE_STATE_DESC ppPsoDesc{};
    ppPsoDesc.pRootSignature = postProcessRootSignature_.Get();
    ppPsoDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
    ppPsoDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
    ppPsoDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();
    ppPsoDesc.PS.BytecodeLength = psBlob->GetBufferSize();
    ppPsoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    ppPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    ppPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    ppPsoDesc.RasterizerState.DepthClipEnable = true;

    // Bloomは加算合成、画面全体の加工は置き換え描画にする。
    ppPsoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    ppPsoDesc.BlendState.RenderTarget[0].BlendEnable = additive ? TRUE : FALSE;
    ppPsoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    ppPsoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
    ppPsoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    ppPsoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    ppPsoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    ppPsoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    ppPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    ppPsoDesc.NumRenderTargets = 1;

    ppPsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    ppPsoDesc.SampleDesc.Count = 1;
    ppPsoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    // 全画面画像の貼り付けには世界内の深度判定を使わない。
    ppPsoDesc.DepthStencilState.DepthEnable = FALSE;
    hr = device->CreateGraphicsPipelineState(&ppPsoDesc, IID_PPV_ARGS(&postProcessPipelineState_));
    Check(hr);
}

void FullscreenPass::Draw(ID3D12GraphicsCommandList* command, ID3D12DescriptorHeap* heap,
                          D3D12_GPU_DESCRIPTOR_HANDLE input, const PostProcessSettings& settings) {
    if (!postProcessPipelineState_) {
        throw std::logic_error("Fullscreen pass is not initialized");
    }

    settings.Validate();
    command->SetGraphicsRootSignature(postProcessRootSignature_.Get());
    command->SetPipelineState(postProcessPipelineState_.Get());
    command->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D12DescriptorHeap* heaps[] = {heap};
    command->SetDescriptorHeaps(1, heaps);
    command->SetGraphicsRootDescriptorTable(0, input);

    // PostProcess.PS.hlslの定数バッファと同じ順序で渡す。
    const float constants[8] = {settings.time,           settings.chromaticAberration,
                                settings.noiseIntensity, float(settings.toneMapping),
                                settings.radialBlur,     settings.centerX,
                                settings.centerY,        0};
    command->SetGraphicsRoot32BitConstants(1, 8, constants, 0);
    command->DrawInstanced(3, 1, 0, 0);
}
