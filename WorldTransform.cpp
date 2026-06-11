#include "WorldTransform.h"
#include <cassert>
#include <cstdint>

void WorldTransform::Initialize(ID3D12Device* device) {
    // 定数バッファの作成 
    uint32_t size = sizeof(ConstBufferDataWorldTransform);
    size = (size + 255) & ~255;

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = size;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE,
        &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&constBuff_)
    );
    assert(SUCCEEDED(hr));

    // マッピング
    constBuff_->Map(0, nullptr, reinterpret_cast<void**>(&constMap_));

    // 初期値を入れておく
    matWorld_ = MakeIdentity4x4();
    constMap_->WVP = MakeIdentity4x4();
    constMap_->World = MakeIdentity4x4();
}

void WorldTransform::UpdateMatrix(const ViewProjection& viewProjection) {
    // スケール・回転・平行移動からワールド行列を計算
    matWorld_ = MakeAffineMatrix(scale_, rotation_, translation_);

    //  親がいれば親の行列を掛ける
    if (parent_) {
        matWorld_ = Multiply(matWorld_, parent_->matWorld_);
    }

    // WVP行列を計算 
    Matrix4x4 wvp = Multiply(matWorld_, Multiply(viewProjection.matView, viewProjection.matProjection));

    // 定数バッファに書き込んでGPUに転送
    constMap_->WVP = wvp;
    constMap_->World = matWorld_;
}