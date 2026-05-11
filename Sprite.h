#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"
#include "DirectXCommon.h"
#include "Transform.h"

// 頂点データの構造体
struct VertexData {
    Vector4 position;
    Vector2 texcoord;
};



class Sprite {
public:
    
    void Initialize(DirectXCommon* dxCommon, D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU);
    void Update();
    void Draw();

    // 位置や大きさを変えるためのゲッター・セッター
   struct Transform& GetTransform() { return transform_; }

private:
    // DirectXの便利クラス
    DirectXCommon* dxCommon_ = nullptr;

    // ▼ 1. 頂点データ関連（スライド 150846）
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    // ▼ 2. 行列データ関連（スライド 150854）
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
    Matrix4x4* transformationMatrixData_ = nullptr;
    struct Transform transform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };

    // 描画するテクスチャのGPUハンドル
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_;

    // リソース作成用の便利関数（Object3dからコピーまたは共有）
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);
};