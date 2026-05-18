#include "Sprite.h"
#include "WindowApp.h" 
#include <cassert>
#include "Transform.h"
#include "Matrix4x4.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#endif
#include "Object3d.h"

void Sprite::Initialize(DirectXCommon* dxCommon, D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU) {
    dxCommon_ = dxCommon;
    textureSrvHandleGPU_ = textureSrvHandleGPU;
    ID3D12Device* device = dxCommon_->GetDevice();

    // ==========================================
    // 頂点バッファの作成
    // ==========================================
    vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * 6);
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(VertexData) * 6;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    VertexData* vertexData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));


    // 1枚目の三角形
    vertexData[0].position = { 0.0f, 360.0f, 0.0f, 1.0f }; // 左下
    vertexData[0].texcoord = { 0.0f, 1.0f };
    vertexData[0].normal = { 0.0f, 0.0f, -1.0f };         // 法線

    vertexData[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };   // 左上
    vertexData[1].texcoord = { 0.0f, 0.0f };
    vertexData[1].normal = { 0.0f, 0.0f, -1.0f };         // 法線

    vertexData[2].position = { 640.0f, 360.0f, 0.0f, 1.0f }; // 右下
    vertexData[2].texcoord = { 1.0f, 1.0f };
    vertexData[2].normal = { 0.0f, 0.0f, -1.0f };         // 法線

    // 2枚目の三角形
    vertexData[3].position = { 0.0f, 0.0f, 0.0f, 1.0f };   // 左上
    vertexData[3].texcoord = { 0.0f, 0.0f };
    vertexData[3].normal = { 0.0f, 0.0f, -1.0f };         // 法線

    vertexData[4].position = { 640.0f, 0.0f, 0.0f, 1.0f };   // 右上
    vertexData[4].texcoord = { 1.0f, 0.0f };
    vertexData[4].normal = { 0.0f, 0.0f, -1.0f };         // 法線

    vertexData[5].position = { 640.0f, 360.0f, 0.0f, 1.0f }; // 右下
    vertexData[5].texcoord = { 1.0f, 1.0f };
    vertexData[5].normal = { 0.0f, 0.0f, -1.0f };         // 法線

    // ==========================================
    // 行列バッファの作成
    // ==========================================
    uint32_t transformMatrixSize = sizeof(TransformationMatrix);
    transformMatrixSize = (transformMatrixSize + 255) & ~255;

    transformationMatrixResource_ = CreateBufferResource(device, transformMatrixSize);
    transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));

    transformationMatrixData_->WVP = MakeIdentity4x4();
    transformationMatrixData_->World = MakeIdentity4x4();


    materialResource_ = CreateBufferResource(device, sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

    // スプライトは基本的に白色
    materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    // スプライトはライティングしないので false
    materialData_->enableLighting = 0;

}

void Sprite::Update() {



#ifdef USE_IMGUI
    ImGui::Begin("Settings");

    ImGui::ColorEdit4("colorSprite", &materialData_->color.x);
    ImGui::DragFloat3("translateSprite", &transform_.translate.x, 1.0f);

    ImGui::End();

 

#endif



    // ==========================================
    // WVP行列の計算
    // ==========================================


    // ワールド行列
    Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

    // ビュー行列
    Matrix4x4 viewMatrix = MakeIdentity4x4();

    // プロジェクション行列
    Matrix4x4 projectionMatrix = MakeOrthographicMatrix(
        0.0f, 0.0f, float(WindowApp::kClientWidth), float(WindowApp::kClientHeight), 0.0f, 100.0f
    );

    Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

    // 行列を掛け合わせて転送
    transformationMatrixData_->WVP = worldViewProjectionMatrix;
    transformationMatrixData_->World = worldMatrix;



}

void Sprite::Draw() {
    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

    // ==========================================
    // 描画コマンドを積む
    // ==========================================

    // 頂点データをセット
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

    // 行列データをセット
    commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());

    // テクスチをセット
    commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU_);


    commandList->DrawInstanced(6, 1, 0, 0);
}


Microsoft::WRL::ComPtr<ID3D12Resource> Sprite::CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {
    D3D12_HEAP_PROPERTIES uploadHeapProperties{};
    uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDesc.Width = sizeInBytes;
    resourceDesc.Height = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = 1;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
    device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
    return resource;
}