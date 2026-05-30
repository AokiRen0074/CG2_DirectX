#pragma once
#include <d3d12.h>
#include <string>
#include <wrl.h>
#include <vector>
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"

class DirectXCommon;

class TextureManager {
public:

    static TextureManager* GetInstance();

    static uint32_t Load(const std::string& filePath);
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureHandle);


    static void StaticInitialize(DirectXCommon* dxCommon);

	static DirectX::ScratchImage LoadTexture(const std::string& filePath);

	static Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);

    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);

    static Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(
        ID3D12Resource* texture,
        const DirectX::ScratchImage& mipImages,
        ID3D12Device* device,
        ID3D12GraphicsCommandList* commandList);

private:
    TextureManager() = default;
    ~TextureManager() = default;
    TextureManager(const TextureManager&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;

    uint32_t LoadInternal(const std::string& filePath);

    // テクスチャのGPUハンドルを記憶しておく配列
    std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> srvHandles_;

    DirectXCommon* dxCommon_ = nullptr;

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> textureResources_;


};