#include "Object3d.h"
#include <cassert>
#include "DirectXCommon.h" 
#include "WindowApp.h"
#include "TextureManager.h"
#include <numbers>
#include "Model.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#endif

struct Vector4 {
	float x, y, z, w;
};

/*--------------------------
初期化
-----------------------------------*/
void Object3d::Initialize(DirectXCommon* dxCommon) {

	dxCommon_ = dxCommon;

	ID3D12Device* device = dxCommon_->GetDevice();

	// RootSignature作成
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;


	// RootParameter作成。複数設定できるので配列
	D3D12_ROOT_PARAMETER rootParameters[4] = {};
	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[0].Descriptor.ShaderRegister = 0;
	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParameters[1].Descriptor.ShaderRegister = 0;

	// DescriptorRangeの設定
	D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	descriptorRange[0].BaseShaderRegister = 0; // t0から始まる
	descriptorRange[0].NumDescriptors = 1;     // 扱うテクスチャの数は1つ
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRVを使う
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// DescriptorTableの設定
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // PixelShaderで使う
	rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);

	// ライト用のCBV
	rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParameters[3].Descriptor.ShaderRegister = 1;

	descriptionRootSignature.pParameters = rootParameters;
	descriptionRootSignature.NumParameters = _countof(rootParameters);

	// Samplerの設定
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // 拡大縮小したときに綺麗にぼかす（バイリニア）
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // UVが1.0を超えたらリピートする
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0; // s0 を使う
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	descriptionRootSignature.pStaticSamplers = staticSamplers;
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	// シリアライズしてバイナリにする
	ID3DBlob* signatureBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature,
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
	if (FAILED(hr)) {
		Logger::Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
		assert(false);
	}

	// バイナリを元に生成
	ID3D12RootSignature* rootSignature = nullptr;
	hr = device->CreateRootSignature(0,
		signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));

	// InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	inputElementDescs[0].SemanticName = "POSITION";
	inputElementDescs[0].SemanticIndex = 0;
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	// テクスチャ座標情報
	inputElementDescs[1].SemanticName = "TEXCOORD";
	inputElementDescs[1].SemanticIndex = 0;

	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	inputElementDescs[2].SemanticName = "NORMAL";
	inputElementDescs[2].SemanticIndex = 0;
	inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

	inputLayoutDesc.pInputElementDescs = inputElementDescs;
	inputLayoutDesc.NumElements = _countof(inputElementDescs);

	// BlendStateの設定
	D3D12_BLEND_DESC blendDesc{};

	// 全ての色要素を書き込む

	blendDesc.RenderTarget[0].RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;

	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};

	// 裏面は表示しない
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;

	// 三角形の中を塗りつぶす
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// Shaderをコンパイルする
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxCommon_->CompilerShader(L"Object3D.VS.hlsl",
		L"vs_6_0", dxCommon_->GetDxcUtils(),
		dxCommon_->GetDxcCompiler(), dxCommon_->GetIncludeHandler()
	);
	assert(vertexShaderBlob != nullptr);


	Microsoft::WRL::ComPtr<IDxcBlob>pixelShaderBlob = dxCommon_->CompilerShader(L"Object3D.PS.hlsl",
		L"ps_6_0", dxCommon_->GetDxcUtils(),
		dxCommon_->GetDxcCompiler(), dxCommon_->GetIncludeHandler()
	);

	assert(pixelShaderBlob != nullptr);

	// PSOを作成する

	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	// Depthの機能を有効化する
	depthStencilDesc.DepthEnable = true;

	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	// 近ければ描画される
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get(); // RootSignature
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;// InputLayout
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
	vertexShaderBlob->GetBufferSize() }; // VertexShader
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),
	pixelShaderBlob->GetBufferSize() };// PixelShader
	graphicsPipelineStateDesc.BlendState = blendDesc; // BlendState
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc; // RasterizerState
	//書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//利用するトポロジ(形状)のタイプ。三角形
	graphicsPipelineStateDesc.PrimitiveTopologyType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//どのように画面に色を打ち込むかの設定(気にしなくて良い)
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	//実際に生成
	ID3D12PipelineState* graphicsPipelineState = nullptr;

	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc,
		IID_PPV_ARGS(&graphicsPipelineState_));
	assert(SUCCEEDED(hr));



	// ==========================================
	// 球体の頂点・インデックスデータの計算
	// ==========================================
	const uint32_t kSubdivision = 16;

	// 頂点数
	const uint32_t kVertexCount = (kSubdivision + 1) * (kSubdivision + 1);
	// インデックス数：分割数 × 分割数 × 6
	const uint32_t kIndexCount = kSubdivision * kSubdivision * 6;

	const float pi = std::numbers::pi_v<float>;

	// 頂点バッファの作成
	vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * kVertexCount);
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * kVertexCount;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);
	VertexData* vertexData = nullptr;
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	//インデックスバッファの作成
	indexResource_ = CreateBufferResource(device, sizeof(uint32_t) * kIndexCount);
	indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
	indexBufferView_.SizeInBytes = sizeof(uint32_t) * kIndexCount;
	indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
	uint32_t* indexData = nullptr;
	indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData));

	// 角度の計算
	const float kLonEvery = pi * 2.0f / float(kSubdivision);
	const float kLatEvery = pi / float(kSubdivision);

	// グリッド状に重複のない頂点だけを敷き詰める
	for (uint32_t latIndex = 0; latIndex <= kSubdivision; ++latIndex) {
		float lat = -pi / 2.0f + kLatEvery * latIndex;
		for (uint32_t lonIndex = 0; lonIndex <= kSubdivision; ++lonIndex) {
			uint32_t index = latIndex * (kSubdivision + 1) + lonIndex; // 1次元配列のインデックス
			float lon = lonIndex * kLonEvery;

			vertexData[index].position = { std::cos(lat) * std::cos(lon), std::sin(lat), std::cos(lat) * std::sin(lon), 1.0f };
			vertexData[index].texcoord = { float(lonIndex) / float(kSubdivision), 1.0f - float(latIndex) / float(kSubdivision) };
			vertexData[index].normal = { std::cos(lat) * std::cos(lon), std::sin(lat), std::cos(lat) * std::sin(lon) };
		}
	}

	// 敷き詰めた頂点をインデックスで結んで四角形を作っていく
	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
			uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;

			// 四角形を構成する4つの頂点の番号を計算
			uint32_t v0 = latIndex * (kSubdivision + 1) + lonIndex;       // 左下
			uint32_t v1 = (latIndex + 1) * (kSubdivision + 1) + lonIndex; // 左上
			uint32_t v2 = latIndex * (kSubdivision + 1) + (lonIndex + 1); // 右下
			uint32_t v3 = (latIndex + 1) * (kSubdivision + 1) + (lonIndex + 1); // 右上

			// 三角形1枚目（左下、左上、右下）
			indexData[start] = v0;
			indexData[start + 1] = v1;
			indexData[start + 2] = v2;

			// 三角形2枚目（右下、左上、右上）
			indexData[start + 3] = v2;
			indexData[start + 4] = v1;
			indexData[start + 5] = v3;
		}
	}

	// 色1つ分（Vector4）のサイズで作る
	materialResources_ = CreateBufferResource(device, sizeof(Vector4));

	uint32_t lightSize = sizeof(DirectionalLight);
	lightSize = (lightSize + 255) & ~255;
	directionalLightResource_ = CreateBufferResource(device, lightSize);

	directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData_));

	// デフォルト値：真下（Yが-1）に向かって白い光を当てる
	directionalLightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightData_->direction = { 0.0f, -1.0f, 0.0f };
	directionalLightData_->intensity = 1.0f;

	// マテリアルにデータを書き込む
	uint32_t materialSize = sizeof(Material);
	materialSize = (materialSize + 255) & ~255;
	materialResources_ = CreateBufferResource(device, materialSize);

	materialResources_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
	materialData_->color = materialColor_;
	materialData_->enableLighting = 1;

	// WVP用のリソースを作る。
	uint32_t transformMatrixSize = sizeof(TransformationMatrix);
	transformMatrixSize = (transformMatrixSize + 255) & ~255; // 256の倍数に切り上げ

	wvpResource_ = CreateBufferResource(device, transformMatrixSize);

	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
	wvpData_->WVP = MakeIdentity4x4();
	wvpData_->World = MakeIdentity4x4();

	// テクスチャ読み込み処理
	DirectX::ScratchImage mipImages = TextureManager::LoadTexture("Resources/uvChecker.png");
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	textureResource_ = TextureManager::CreateTextureResource(device, metadata);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource =
		TextureManager::UploadTextureData(textureResource_.Get(), mipImages, device, dxCommon_->GetCommandList());

	dxCommon_->FlushCommandList();

	// 二枚目のテクスチャを読み込む
	DirectX::ScratchImage mipImages2 = TextureManager::LoadTexture("Resources/monsterBall.png");
	const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();

	// ここで 2枚目のテクスチャ本体を作る
	textureResource2_ = TextureManager::CreateTextureResource(device, metadata2);

	Model model;
	ModelData modelData = model.LoadObjectFile("resources", "plane.obj");

	// 頂点リソースを作る（サイズは「頂点1個分のサイズ × 頂点の数」）
	vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * modelData.vertices.size());

	// 頂点バッファビューを作成する
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	// 頂点リソースにデータを書き込む
	VertexData* vertexData = nullptr;
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	// 今までは forループ で1個ずつ書いていましたが、今回は memcpy で配列ごと一気にコピーします
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());









	// VRAMにデータを転送して待つ
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource2 = TextureManager::UploadTextureData(textureResource2_.Get(), mipImages2, device, dxCommon_->GetCommandList());
	dxCommon_->FlushCommandList();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2{};
	srvDesc2.Format = metadata2.format; // さっき定義した metadata2 を使う
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

	// どこに作るか（DescriptorHeapの場所）を決める
	ID3D12DescriptorHeap* srvHeap = dxCommon_->GetSrvDescriptorHeap();
	uint32_t srvSize = dxCommon_->GetDescriptorSizeSRV();

	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = dxCommon_->GetCPUDescriptorHandle(srvHeap, srvSize, 2);
	textureSrvHandleGPU2_ = dxCommon_->GetGPUDescriptorHandle(srvHeap, srvSize, 2);

	// index=2 の場所にSRV（本）を登録する
	// さっき定義した textureResource2_ を使う
	device->CreateShaderResourceView(textureResource2_.Get(), &srvDesc2, textureSrvHandleCPU2);

	// SRV用のヒープをDirectXCommonから取得
	ID3D12DescriptorHeap* srvDescriptorHeap = dxCommon_->GetSrvDescriptorHeap();

	// metadataを基にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	// SRVを作成するDescriptorHeapの場所を決める
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	textureSrvHandleGPU_ = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(); // メンバ変数に保存

	// 先頭はImGuiが使っているので、その次を使う
	UINT incrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	textureSrvHandleCPU.ptr += incrementSize;
	textureSrvHandleGPU_.ptr += incrementSize;
	// SRVの生成
	device->CreateShaderResourceView(textureResource_.Get(), &srvDesc, textureSrvHandleCPU);

	materialData_->uvTransform = MakeIdentity4x4();


}


void Object3d::Update() {
	// Y軸を毎フレーム少しずつ回転させる
	transform_.rotate.y += 0.03f;

	// アフィン変換行列を作る
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);

	Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform_.scale, cameraTransform_.rotate, cameraTransform_.translate);
	Matrix4x4 viewMatrix = Inverse(cameraMatrix);

	Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(WindowApp::kClientWidth) / float(WindowApp::kClientHeight), 0.1f, 100.0f);

	Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));

	// 計算した行列を、GPUに送るデータに上書きする
	wvpData_->WVP = worldViewProjectionMatrix;
	wvpData_->World = worldMatrix;

#ifdef USE_IMGUI
	ImGui::Begin("Settings");

	// ==========================================
	// カメラ設定
	// ==========================================
	if (ImGui::TreeNode("Camera")) {
		ImGui::DragFloat3("CameraTranslate", &cameraTransform_.translate.x, 0.01f);
		ImGui::DragFloat("CameraRotateX", &cameraTransform_.rotate.x, 0.01f, 0.0f, 0.0f, "%.3f deg");
		ImGui::DragFloat("CameraRotateY", &cameraTransform_.rotate.y, 0.01f, 0.0f, 0.0f, "%.3f deg");
		ImGui::DragFloat("CameraRotateZ", &cameraTransform_.rotate.z, 0.01f, 0.0f, 0.0f, "%.3f deg");

		ImGui::TreePop(); 
	}

	// ==========================================
	// 球体設定
	// ==========================================
	if (ImGui::TreeNode("Sphere Settings")) {
		ImGui::ColorEdit4("Material Color", &materialColor_.x);
		ImGui::Checkbox("useMonsterBall", &useMonsterBall_);

		// さらに階層を深く
		if (ImGui::TreeNode("UV Transform")) {
			ImGui::DragFloat2("Translate", &uvTransform_.translate.x, 0.01f, -10.0f, 10.0f);
			ImGui::DragFloat2("Scale", &uvTransform_.scale.x, 0.01f, -10.0f, 10.0f);
			ImGui::SliderAngle("Rotate", &uvTransform_.rotate.z);
			ImGui::TreePop();
		}

		ImGui::TreePop();
	}

	// ==========================================
	// 平行光源
	// ==========================================
	if (ImGui::TreeNode("Directional Light")) {
		// enableLightingはint32_tなのでbool変換
		bool isLighting = (materialData_->enableLighting != 0);
		if (ImGui::Checkbox("enableLighting", &isLighting)) {
			materialData_->enableLighting = isLighting ? 1 : 0;
		}

		ImGui::ColorEdit4("LightColor", &directionalLightData_->color.x);

		if (ImGui::DragFloat3("LightDirection", &directionalLightData_->direction.x, 0.01f, -1.0f, 1.0f)) {
			float len = std::sqrt(
				directionalLightData_->direction.x * directionalLightData_->direction.x +
				directionalLightData_->direction.y * directionalLightData_->direction.y +
				directionalLightData_->direction.z * directionalLightData_->direction.z
			);
			if (len != 0.0f) {
				directionalLightData_->direction.x /= len;
				directionalLightData_->direction.y /= len;
				directionalLightData_->direction.z /= len;
			}
		}
		ImGui::DragFloat("Intensity", &directionalLightData_->intensity, 0.01f);

		ImGui::TreePop();
	}

	ImGui::End();

	// 最後に色データを更新
	materialData_->color = materialColor_;
#endif

	Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransform_.scale);
	uvTransformMatrix = Multiply(uvTransformMatrix, MakeRotateZMatrix(uvTransform_.rotate.z));
	uvTransformMatrix = Multiply(uvTransformMatrix, MakeTranslateMatrix(uvTransform_.translate));

	materialData_->uvTransform = uvTransformMatrix;

}


/*----------------------------------------
描画
---------------------------------------*/
void Object3d::Draw() {

	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

	//ビューポート
	D3D12_VIEWPORT viewport{};
	// クライアント領域のサイズと一緒にして画面全体に表示
	viewport.Width = WindowApp::kClientWidth;
	viewport.Height = WindowApp::kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	//シザー矩形
	D3D12_RECT scissorRect{};
	//基本的にビューポートと同じ矩形が構成されるようにする
	scissorRect.left = 0;
	scissorRect.right = WindowApp::kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = WindowApp::kClientHeight;

	commandList->RSSetViewports(1, &viewport); // Viewport &RE
	commandList->RSSetScissorRects(1, &scissorRect);
	//RootSignatureを設定
	commandList->SetGraphicsRootSignature(rootSignature_.Get());
	commandList->SetPipelineState(graphicsPipelineState_.Get());
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

	// インデックスバッファをセット
	commandList->IASetIndexBuffer(&indexBufferView_);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけば良い
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D12DescriptorHeap* descriptorHeaps[] = { dxCommon_->GetSrvDescriptorHeap() };
	commandList->SetDescriptorHeaps(1, descriptorHeaps);

	commandList->SetGraphicsRootConstantBufferView(0, materialResources_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootDescriptorTable(2, useMonsterBall_ ? textureSrvHandleGPU2_ : textureSrvHandleGPU_);
	commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource_->GetGPUVirtualAddress());

	commandList->DrawIndexedInstanced(1536, 1, 0, 0, 0);
}

/*--------------------------
Resource作成の関数化
------------------------------*/


Microsoft::WRL::ComPtr<ID3D12Resource> Object3d::CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {
	// 頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // UploadHeap

	// 頂点リソースの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	//サイズをセットする
	resourceDesc.Width = sizeInBytes;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// 実際にリソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));

	return resource;
}