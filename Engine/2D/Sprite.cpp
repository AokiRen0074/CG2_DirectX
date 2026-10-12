#include "Sprite.h"
#include "WindowApp.h" 
#include <cassert>
#include <cmath>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include "Transform.h"
#include "Matrix4x4.h"
#include "2D/TextureManager.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#endif
#include "Object3d.h"


namespace {

void CheckResult(HRESULT result) {
    if (FAILED(result)) {
        throw std::runtime_error("Failed to create sprite rendering resources");
    }
}

}

DirectXCommon* Sprite::sDxCommon_ = nullptr;

void Sprite::StaticInitialize(DirectXCommon* dxCommon) {
    sDxCommon_ = dxCommon;
}

Sprite* Sprite::Create(uint32_t textureHandle, Vector2 position) {
    // メモリを確保
    auto sprite = std::make_unique<Sprite>();

    // クラスが記憶しているdxCommonを使って初期化
    sprite->Initialize(sDxCommon_, textureHandle);

    //  座標をセット
    sprite->SetPosition(position);

    // 完成品を返す
    sprite->Update();
    return sprite.release();
}

void Sprite::InitializePipeline() {
    ID3D12Device* device = dxCommon_->GetDevice();


    // テクスチャを参照するための範囲
    D3D12_DESCRIPTOR_RANGE range{};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = 1;
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // マテリアル・行列・テクスチャの受け渡し
    D3D12_ROOT_PARAMETER parameters[3]{};
    parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    parameters[2].DescriptorTable.NumDescriptorRanges = 1;
    parameters[2].DescriptorTable.pDescriptorRanges = &range;

    // 画像の端を繰り返さないサンプラー
    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    // RootSignatureの作成
    D3D12_ROOT_SIGNATURE_DESC rootDesc{};
    rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootDesc.NumParameters = _countof(parameters);
    rootDesc.pParameters = parameters;
    rootDesc.NumStaticSamplers = 1;
    rootDesc.pStaticSamplers = &sampler;
    Microsoft::WRL::ComPtr<ID3DBlob> signature, errors;
    CheckResult(D3D12SerializeRootSignature(&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errors));
    CheckResult(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature_)));
    auto vs = dxCommon_->CompilerShader(L"Resources/Shaders/Object3d.VS.hlsl", L"vs_6_0",
        dxCommon_->GetDxcUtils(), dxCommon_->GetDxcCompiler(), dxCommon_->GetIncludeHandler());
    auto ps = dxCommon_->CompilerShader(L"Resources/Shaders/Sprite.PS.hlsl", L"ps_6_0",
        dxCommon_->GetDxcUtils(), dxCommon_->GetDxcCompiler(), dxCommon_->GetIncludeHandler());

    // InputLayout
    D3D12_INPUT_ELEMENT_DESC elements[3]{};
    elements[0].SemanticName = "POSITION";
    elements[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    elements[1].SemanticName = "TEXCOORD";
    elements[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    elements[2].SemanticName = "NORMAL";
    elements[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    for (auto& element : elements) {
        element.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    }

    // スプライト専用の描画設定
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
    desc.pRootSignature = rootSignature_.Get();
    desc.InputLayout = { elements, _countof(elements) };
    desc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
    desc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
    desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    desc.RasterizerState.DepthClipEnable = TRUE;
    // 3Dの深度に影響されず、呼び出した順に重ねる。
    desc.DepthStencilState.DepthEnable = FALSE;
    desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    auto& blend = desc.BlendState.RenderTarget[0];
    blend.BlendEnable = TRUE;
    blend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blend.BlendOp = D3D12_BLEND_OP_ADD;
    blend.SrcBlendAlpha = D3D12_BLEND_ONE;
    blend.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    desc.NumRenderTargets = 1;
    desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    desc.SampleDesc.Count = 1;
    desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    CheckResult(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipelineState_)));
}

void Sprite::Initialize(DirectXCommon* dxCommon, uint32_t textureHandle) {
    if (!dxCommon) {
        throw std::invalid_argument("Sprite requires DirectXCommon initialization");
    }

    dxCommon_ = dxCommon;
    textureHandle_ = textureHandle;
    ID3D12Device* device = dxCommon_->GetDevice();

    // 読み込み済みの画像サイズを取得し、初期状態では画像全体を使う
    const auto textureDesc = TextureManager::GetInstance()->GetResourceDesc(textureHandle_);
    textureSize_ = { static_cast<float>(textureDesc.Width), static_cast<float>(textureDesc.Height) };
    textureLeftTop_ = { 0.0f, 0.0f };
    textureRectSize_ = textureSize_;
    SetScale(textureSize_);

    InitializePipeline();

    // ==========================================
    // 頂点バッファの作成
    // ==========================================
    vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * 4);
    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(VertexData) * 4;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    VertexData* vertexData = nullptr;
    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));


    // 三角形
    vertexData[0].position = { 0.0f, 360.0f, 0.0f, 1.0f }; // 左下
    vertexData[0].texcoord = { 0.0f, 1.0f };
    vertexData[0].normal = { 0.0f, 0.0f, -1.0f };          // 法線

    vertexData[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };   // 左上
    vertexData[1].texcoord = { 0.0f, 0.0f };
    vertexData[1].normal = { 0.0f, 0.0f, -1.0f };          // 法線

    vertexData[2].position = { 640.0f, 360.0f, 0.0f, 1.0f }; // 右下
    vertexData[2].texcoord = { 1.0f, 1.0f };
    vertexData[2].normal = { 0.0f, 0.0f, -1.0f };           // 法線

    vertexData[3].position = { 640.0f, 0.0f, 0.0f, 1.0f };   // 右上
    vertexData[3].texcoord = { 1.0f, 0.0f };
    vertexData[3].normal = { 0.0f, 0.0f, -1.0f };           //　法線

    // 四角
    vertexData[0].position = { -0.5f,  0.5f, 0.0f, 1.0f }; // 左下
    vertexData[0].texcoord = { 0.0f, 1.0f };

    vertexData[1].position = { -0.5f, -0.5f, 0.0f, 1.0f }; // 左上
    vertexData[1].texcoord = { 0.0f, 0.0f };

    vertexData[2].position = { 0.5f,  0.5f, 0.0f, 1.0f }; // 右下
    vertexData[2].texcoord = { 1.0f, 1.0f };

    vertexData[3].position = { 0.5f, -0.5f, 0.0f, 1.0f }; // 右上
    vertexData[3].texcoord = { 1.0f, 0.0f };


    /*-------------------------------
    インデックスバッファの作成
    ---------------------------------------*/
    indexResourceSprite_ = CreateBufferResource(device, sizeof(uint32_t) * 6);

    indexBufferViewSprite_.BufferLocation = indexResourceSprite_->GetGPUVirtualAddress();
    indexBufferViewSprite_.SizeInBytes = sizeof(uint32_t) * 6;
    indexBufferViewSprite_.Format = DXGI_FORMAT_R32_UINT;

    uint32_t* indexDataSprite = nullptr;
    indexResourceSprite_->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));

    // 頂点を結ぶ順番
    // 三角形1枚目
    indexDataSprite[0] = 0; indexDataSprite[1] = 1; indexDataSprite[2] = 2;
    // 三角形2枚目
    indexDataSprite[3] = 1; indexDataSprite[4] = 3; indexDataSprite[5] = 2;


    // ==========================================
    // 行列バッファの作成
    // ==========================================
    uint32_t transformMatrixSize = sizeof(TransformationMatrix);
    transformMatrixSize = (transformMatrixSize + 255) & ~255;

    transformationMatrixResource_ = CreateBufferResource(device, transformMatrixSize);
    transformationMatrixResource_->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData_));

    transformationMatrixData_->WVP = MakeIdentity4x4();
    transformationMatrixData_->World = MakeIdentity4x4();


    materialResource_ = CreateBufferResource(device, (sizeof(Material) + 255) & ~size_t(255));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

    // スプライトは基本的に白色
    materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    // スプライトはライティングしないので false
    materialData_->enableLighting = 0;

    materialData_->uvTransform = MakeIdentity4x4();

    Update();



}

void Sprite::Update() {


    /*
#ifdef USE_IMGUI
    ImGui::Begin("Settings");

    // ==========================================
    // スプライト設定
    // ==========================================
    if (ImGui::TreeNode("Sprite Settings")) {
        ImGui::ColorEdit4("Color", &materialData_->color.x);
        ImGui::DragFloat3("Translate", &transform_.translate.x, 1.0f);

        if (ImGui::TreeNode("UV Transform")) {
            ImGui::DragFloat2("Translate", &uvTransformSprite_.translate.x, 0.01f, -10.0f, 10.0f);
            ImGui::DragFloat2("Scale", &uvTransformSprite_.scale.x, 0.01f, -10.0f, 10.0f);
            ImGui::SliderAngle("Rotate", &uvTransformSprite_.rotate.z);
            ImGui::TreePop();
        }

        ImGui::TreePop();
    }

    ImGui::End();
#endif
*/

    //UVTransform行列の計算
    Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite_.scale);
    uvTransformMatrix = Multiply(uvTransformMatrix, MakeRotateZMatrix(uvTransformSprite_.rotate.z));
    uvTransformMatrix = Multiply(uvTransformMatrix, MakeTranslateMatrix(uvTransformSprite_.translate));

    // 切り取り位置と大きさを、画像サイズからUV座標へ変換する
    Vector3 cropScale = {
        textureRectSize_.x / textureSize_.x,
        textureRectSize_.y / textureSize_.y,
        1.0f
    };
    Vector3 cropOffset = {
        textureLeftTop_.x / textureSize_.x,
        textureLeftTop_.y / textureSize_.y,
        0.0f
    };
    Matrix4x4 cropMatrix = Multiply(MakeScaleMatrix(cropScale), MakeTranslateMatrix(cropOffset));
    uvTransformMatrix = Multiply(uvTransformMatrix, cropMatrix);

    // GPUへ送るデータに代入
    materialData_->uvTransform = uvTransformMatrix;



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

    // 直前の3D描画やBloom処理に依存しないよう、描画設定をセットする
    commandList->SetGraphicsRootSignature(rootSignature_.Get());
    commandList->SetPipelineState(pipelineState_.Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = { dxCommon_->GetSrvDescriptorHeap() };
    commandList->SetDescriptorHeaps(1, descriptorHeaps);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D12_VIEWPORT viewport{};
    viewport.Width = float(WindowApp::kClientWidth);
    viewport.Height = float(WindowApp::kClientHeight);
    viewport.MaxDepth = 1.0f;

    D3D12_RECT scissorRect = { 0, 0, WindowApp::kClientWidth, WindowApp::kClientHeight };
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

    // ==========================================
    // 描画コマンドを積む
    // ==========================================

    // 頂点データをセット
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

    // インデックスデータをセット
    commandList->IASetIndexBuffer(&indexBufferViewSprite_);
    commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());

    // 行列データをセット
    commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());

    // テクスチをセット
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = TextureManager::GetInstance()->GetSrvHandleGPU(textureHandle_);
    commandList->SetGraphicsRootDescriptorTable(2, gpuHandle);


    commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);
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
    CheckResult(device->CreateCommittedResource(
        &uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource)));
    return resource;
}

void Sprite::SetPosition(const Vector2& position) {
    transform_.translate.x = position.x;
    transform_.translate.y = position.y;
    //  transform_.translate.z = 0.0f; 
}

void Sprite::SetScale(const Vector2& scale) {
    transform_.scale.x = scale.x;
    transform_.scale.y = scale.y;
}

void Sprite::SetRotation(float rotation) {
    transform_.rotate.z = rotation;
}

// 切り取り範囲の指定
bool Sprite::SetTextureRect(const Vector2& leftTop, const Vector2& size) {
    if (!std::isfinite(leftTop.x) || !std::isfinite(leftTop.y) ||
        !std::isfinite(size.x) || !std::isfinite(size.y) ||
        leftTop.x < 0.0f || leftTop.y < 0.0f ||
        size.x <= 0.0f || size.y <= 0.0f ||
        leftTop.x >= textureSize_.x || leftTop.y >= textureSize_.y) {
        return false;
    }

    textureLeftTop_ = leftTop;
    textureRectSize_ = {
        (std::min)(size.x, textureSize_.x - leftTop.x),
        (std::min)(size.y, textureSize_.y - leftTop.y)
    };

    return true;
}
