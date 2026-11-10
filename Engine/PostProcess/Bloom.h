#pragma once
#include "FullscreenPass.h"

// 発光対象のHDR描画 → 横・縦のガウシアンぼかし → 画面への加算合成。
// PreDraw〜PostDrawではHDR対応PSOを使う。破棄前には呼び出し側でGPU完了を待つ。
class Bloom {
public:
    void Initialize(DirectXCommon* dxCommon, int width, int height);
    void PreDraw();
    void PostDraw();
    void Execute();
    void DrawResult();
    void SetThreshold(float threshold);
    void SetIntensity(float intensity);
    void SetCompositeSettings(const PostProcessSettings& settings);
    void DrawImGui();

private:
    enum Descriptor : UINT { InputSRV, ResultUAV, ResultSRV, ScratchUAV, ScratchSRV, Count };

    D3D12_GPU_DESCRIPTOR_HANDLE Handle(UINT index) const;
    DirectXCommon* dxCommon_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    float threshold_ = 0.75f;
    float intensity_ = 1.05f;

    // 描画・計算・合成の呼び出し順を検証する。
    bool drawing_ = false;
    bool inputReady_ = false;
    bool resultReady_ = false;
    PostProcessSettings compositeSettings_;
    FullscreenPass composite_;

    Microsoft::WRL::ComPtr<ID3D12Resource> hdrTexture_;
    Microsoft::WRL::ComPtr<ID3D12Resource> result_;
    Microsoft::WRL::ComPtr<ID3D12Resource> scratch_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_;
};
