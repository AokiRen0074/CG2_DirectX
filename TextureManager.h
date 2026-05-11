#pragma once
#include <d3d12.h>
#include <string>
#include <wrl.h>
#include "externals/DirectXTex/DirectXTex.h"

class TextureManager {
public:

	static DirectX::ScratchImage LoadTexture(const std::string& filePath);

	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);

	static void UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages);

private:

};