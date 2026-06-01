#include "TextureManager.h"
#include "Logger.h"
#include <cassert>
#include "Base/DirectXCommon.h"


void TextureManager::StaticInitialize(DirectXCommon* dxCommon) {
    GetInstance()->dxCommon_ = dxCommon;
}

TextureManager* TextureManager::GetInstance() {
    static TextureManager instance;
    return &instance;
}

void TextureManager::Finalize() {

    GetInstance()->textureResources_.clear();
    GetInstance()->srvHandles_.clear();
}

uint32_t TextureManager::Load(const std::string& filePath) {
    return GetInstance()->LoadInternal(filePath);
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(uint32_t textureHandle) {
    assert(textureHandle < srvHandles_.size()); // 範囲外アクセス防止
    return srvHandles_[textureHandle];
}

uint32_t TextureManager::LoadInternal(const std::string& filePath) {
    // 画像ファイルを読み込んでミップマップを生成
    DirectX::ScratchImage mipImages = LoadTexture(filePath);
    const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

    // テクスチャリソースの作成
    Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = CreateTextureResource(dxCommon_->GetDevice(), metadata);

    // データをVRAMに転送
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = UploadTextureData(
      
     textureResource.Get(), mipImages, dxCommon_->GetDevice(), dxCommon_->GetCommandList());
    dxCommon_->FlushCommandList();

 
    //  管理用配列に保存
    textureResources_.push_back(textureResource);

    // ==========================================
    // SRVの作成
    // ==========================================
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = metadata.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

    ID3D12Device* device = dxCommon_->GetDevice();

    ID3D12DescriptorHeap* srvHeap = dxCommon_->GetSrvDescriptorHeap();
    uint32_t srvSize = dxCommon_->GetDescriptorSizeSRV();
    uint32_t srvIndex = static_cast<uint32_t>(srvHandles_.size()) + 1;

    // CPUとGPUのハンドル（場所）を計算
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = dxCommon_->GetCPUDescriptorHandle(srvHeap, srvSize, srvIndex);
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = dxCommon_->GetGPUDescriptorHandle(srvHeap, srvSize, srvIndex);

    // SRVを作成！
    device->CreateShaderResourceView(textureResource.Get(), &srvDesc, cpuHandle);

    srvHandles_.push_back(gpuHandle);
    return static_cast<uint32_t>(srvHandles_.size() - 1);

}

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
    // metadataを基にResourceの設定
    D3D12_RESOURCE_DESC resourceDesc{};

    resourceDesc.Width = UINT(metadata.width);             // Textureの幅
    resourceDesc.Height = UINT(metadata.height);           // Textureの高さ
    resourceDesc.MipLevels = UINT16(metadata.mipLevels);   // mipmapの数
    resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize); // 奥行き
    resourceDesc.Format = metadata.format;                 // TextureのFormat
    resourceDesc.SampleDesc.Count = 1;                     // サンプリングカウント。1固定。
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, // データ転送される設定
        nullptr,
        IID_PPV_ARGS(&resource)
    );
    assert(SUCCEEDED(hr));
    return resource;
}

Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height) {
    // 生成するResourceの設定
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Width = width;                                   // Textureの幅
    resourceDesc.Height = height;                                 // Textureの高さ
    resourceDesc.MipLevels = 1;                                   // mipmapの数
    resourceDesc.DepthOrArraySize = 1;                            // 奥行き or 配列Textureの配列数
    resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;          // 深度(Depth)24bit、ステンシル(Stencil)8bitのフォーマット
    resourceDesc.SampleDesc.Count = 1;                            // サンプリングカウント。1固定。
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;  // 2次元
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // 深度バッファとして使うよ！というフラグ

    // 利用するHeapの設定
    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;                // VRAM上に作る

    // 深度値のクリア最適化設定
    D3D12_CLEAR_VALUE depthClearValue{};
    depthClearValue.DepthStencil.Depth = 1.0f;                    // 1.0f（一番遠い距離）で初期化する
    depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;       // リソースと同じフォーマットにする

    // Resourceの生成
    Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &heapProperties,                  // Heapの設定
        D3D12_HEAP_FLAG_NONE,             // Heapの特殊な設定
        &resourceDesc,                    // Resourceの設定
        D3D12_RESOURCE_STATE_DEPTH_WRITE, // 深度値を書き込む状態にしておく
        &depthClearValue,                 // クリア最適値
        IID_PPV_ARGS(&resource)           // 作成するResourceポインタへのポインタ
    );
    assert(SUCCEEDED(hr));

    return resource;
}

/*-------------------------------------------
テクスチャリソースにデータを転送する
-------------------------------------------------*/
Microsoft::WRL::ComPtr<ID3D12Resource> TextureManager::UploadTextureData(
    ID3D12Resource* texture, const DirectX::ScratchImage& mipImages,
    ID3D12Device* device, ID3D12GraphicsCommandList* commandList)
{
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    DirectX::PrepareUpload(device, mipImages.GetImages(), mipImages.GetImageCount(), mipImages.GetMetadata(), subresources);
    uint64_t intermediateSize = GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));

    // 中間リソース（UploadHeap）の作成
    Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
    D3D12_HEAP_PROPERTIES uploadHeapProps{};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC bufferDesc{};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = intermediateSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    device->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&intermediateResource));

    // データ転送コマンドを積む
    UpdateSubresources(commandList, texture, intermediateResource.Get(), 0, 0, UINT(subresources.size()), subresources.data());

    // 転送完了後、シェーダーで読めるようにステートを変更するバリアを張る
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = texture;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
    commandList->ResourceBarrier(1, &barrier);

    return intermediateResource; // 転送が終わるまで消えないように返す
}
