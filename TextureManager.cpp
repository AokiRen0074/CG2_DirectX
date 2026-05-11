#include "TextureManager.h"
#include "Logger.h"
#include <cassert>

/*---------------------------------
テクスチャデータを読む関数
---------------------------------------*/
DirectX::ScratchImage TextureManager::LoadTexture(const std::string& filePath) {
    DirectX::ScratchImage image{};
    std::wstring filePathW = Logger::ConvertString(filePath);

    HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
    assert(SUCCEEDED(hr));

    // ミップマップを自動生成する
    DirectX::ScratchImage mipImages{};
    hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImages);
    assert(SUCCEEDED(hr));

    // ミップマップ付きのデータを返す
    return mipImages;
}

/*-------------------------------------
読み込んだ画像のサイズに合わせてテクスチャリソースを作成する関数
-----------------------------------------*/
Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata) {
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Width = UINT(metadata.width);             // Textureの幅
    resourceDesc.Height = UINT(metadata.height);           // Textureの高さ
    resourceDesc.MipLevels = UINT16(metadata.mipLevels);   // mipmapの数
    resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize); // 奥行き or 配列の数
    resourceDesc.Format = metadata.format;                 // Textureのフォーマット
    resourceDesc.SampleDesc.Count = 1;                     // サンプリングカウント。
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension); // 2次元か3次元か

    // 利用するHeapの設定
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM;                        // 細かい設定を行う
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;          // プロセッサの近くに配置

    //  Resourceを生成
    Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &heapProperties,                 // Heapの設定
        D3D12_HEAP_FLAG_NONE,            // Heapの特殊な設定
        &resourceDesc,                   // Resourceの設定
        D3D12_RESOURCE_STATE_GENERIC_READ, // 初回のResourceState
        nullptr,                         // Clear最適値（使わない）
        IID_PPV_ARGS(&resource)
    );
    assert(SUCCEEDED(hr));

    return resource;
}

/*-------------------------------------------
テクスチャリソースにデータを転送する
-------------------------------------------------*/
void TextureManager::UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages) {
    // Meta情報を取得
    const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

    // 全MipMapについて
    for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel) {
        // MipMapLevelを指定して各Imageを取得
        const DirectX::Image* img = mipImages.GetImage(mipLevel, 0, 0);

        // Textureに転送（GPUのキャンバスに書き込む）
        HRESULT hr = texture->WriteToSubresource(
            UINT(mipLevel),
            nullptr,              // 全領域へコピー
            img->pixels,          // 元データのアドレス
            UINT(img->rowPitch),  // 1ラインのサイズ
            UINT(img->slicePitch) // 1枚のサイズ
        );
        assert(SUCCEEDED(hr));
    }
}
