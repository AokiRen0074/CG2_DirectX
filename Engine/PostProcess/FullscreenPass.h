#pragma once
#include "Base/DirectXCommon.h"
#include "PostProcessSettings.h"

// 入力画像を全画面へ描く共通処理。画像の保存やBloomのぼかしは呼び出し側が担当する。
class FullscreenPass {
public:
    void Initialize(DirectXCommon* dx, bool additive);
    void Draw(ID3D12GraphicsCommandList* command, ID3D12DescriptorHeap* heap, D3D12_GPU_DESCRIPTOR_HANDLE input,
              const PostProcessSettings& settings);

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> postProcessRootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> postProcessPipelineState_;
};
