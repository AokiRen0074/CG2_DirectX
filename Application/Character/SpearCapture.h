#pragma once
#include "DirectXCommon.h"
#include "externals/DirectXTex/DirectXTex.h"
#include <filesystem>
#include <wincodec.h>

// 開発用の画面確認。実際に描画したバックバッファをPNGへ保存する。
inline bool CaptureSpearFrame(DirectXCommon *graphics, UINT bufferIndex,
                              const std::filesystem::path &path) {
    Microsoft::WRL::ComPtr<ID3D12Resource> buffer;
    if (FAILED(graphics->GetSwapChain()->GetBuffer(bufferIndex, IID_PPV_ARGS(&buffer)))) {
        return false;
    }

    DirectX::ScratchImage captured;
    if (FAILED(DirectX::CaptureTexture(graphics->GetCommandQueue(), buffer.Get(), false, captured,
                                       D3D12_RESOURCE_STATE_PRESENT,
                                       D3D12_RESOURCE_STATE_PRESENT))) {
        return false;
    }

    const DirectX::Image *image = captured.GetImage(0, 0, 0);
    return image && SUCCEEDED(DirectX::SaveToWICFile(*image, DirectX::WIC_FLAGS_NONE,
                                                     GUID_ContainerFormatPng, path.c_str()));
}
