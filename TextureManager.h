#pragma once
#include <d3d12.h>
#include <string>
#include <wrl.h>
#include <vector>
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"

class TextureManager {
public:

	static DirectX::ScratchImage LoadTexture(const std::string& filePath);

	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);

    static Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(
        ID3D12Resource* texture,
        const DirectX::ScratchImage& mipImages,
        ID3D12Device* device,
        ID3D12GraphicsCommandList* commandList);

private:

};