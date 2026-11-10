#pragma once
#include "FullscreenPass.h"

// バックバッファをコピーし、PSで画面全体を加工する。
// UIを加工しない場合はUI描画前にApplyする。加工が無効ならGPU処理を省略する。
class ScenePostProcess {
public:
    void Initialize(DirectXCommon* dxCommon);
    void SetSettings(const PostProcessSettings& settings);
    void SetRadialBlur(float strength, float centerX, float centerY);
    bool Apply();

private:
    DirectXCommon* dxCommon_ = nullptr;
    PostProcessSettings settings_;
    FullscreenPass fullscreen_;
    Microsoft::WRL::ComPtr<ID3D12Resource> scene_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap_;
};
