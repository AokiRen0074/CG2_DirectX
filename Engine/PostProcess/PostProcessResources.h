#pragma once
#include "Base/DirectXCommon.h"
#include <stdexcept>

// ポストプロセスで共用するリソース生成と状態切り替え。
namespace PostProcessResources {
inline void Check(HRESULT hr) {
    if (FAILED(hr)) {
        throw std::runtime_error("Post-process DirectX operation failed");
    }
}

inline void Validate(DirectXCommon* dx, int width, int height) {
    if (!dx || !dx->GetDevice() || width <= 0 || height <= 0 || width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
        height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION) {
        throw std::invalid_argument("Invalid post-process device or dimensions");
    }
}

inline Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Heap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type,
                                                         UINT count, bool visible) {
    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Type = type;
    desc.NumDescriptors = count;
    desc.Flags = visible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> result;
    Check(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&result)));
    return result;
}

// 1を超える発光色を保持できる浮動小数点のHDR画像を作る。
inline Microsoft::WRL::ComPtr<ID3D12Resource> Texture(ID3D12Device* device, int width, int height,
                                                      D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES state) {
    D3D12_HEAP_PROPERTIES heap{};
    heap.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    desc.SampleDesc.Count = 1;
    desc.Flags = flags;
    Microsoft::WRL::ComPtr<ID3D12Resource> result;
    Check(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr, IID_PPV_ARGS(&result)));
    return result;
}

inline void Transition(ID3D12GraphicsCommandList* command, ID3D12Resource* resource, D3D12_RESOURCE_STATES before,
                       D3D12_RESOURCE_STATES after) {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    command->ResourceBarrier(1, &barrier);
}
} // namespace PostProcessResources
