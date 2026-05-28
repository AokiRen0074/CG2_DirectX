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
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxCommon_->CompilerShader(L"Resources/Shaders/Object3D.VS.hlsl",
		L"vs_6_0", dxCommon_->GetDxcUtils(),
		dxCommon_->GetDxcCompiler(), dxCommon_->GetIncludeHandler()
	);
	assert(vertexShaderBlob != nullptr);


	Microsoft::WRL::ComPtr<IDxcBlob>pixelShaderBlob = dxCommon_->CompilerShader(L"Resources/Shaders/Object3D.PS.hlsl",
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


	/*
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

	*/


	// ==========================================
	// OBJモデルデータの読み込み
	// ==========================================

	ModelData modelData = LoadObjectFile("Resources", "multiMaterial.obj");

	// SRVの割り当て用インデックス
	uint32_t srvIndex = 1;
	ID3D12DescriptorHeap* srvHeap = dxCommon_->GetSrvDescriptorHeap();
	uint32_t srvSize = dxCommon_->GetDescriptorSizeSRV();


	// ==========================================
	//  メッシュの数だけバッファを作るループ
	// ==========================================
	for (auto& mesh : modelData.meshes) {
		MeshResource meshRes;
		meshRes.vertexCount = UINT(mesh.vertices.size());

		// 頂点バッファの作成
		meshRes.vertexResource = CreateBufferResource(device, sizeof(VertexData) * meshRes.vertexCount);
		meshRes.vertexBufferView.BufferLocation = meshRes.vertexResource->GetGPUVirtualAddress();
		meshRes.vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * meshRes.vertexCount);
		meshRes.vertexBufferView.StrideInBytes = sizeof(VertexData);

		VertexData* vertexData = nullptr;
		meshRes.vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
		std::memcpy(vertexData, mesh.vertices.data(), sizeof(VertexData) * meshRes.vertexCount);

		// マテリアルバッファの作成
		uint32_t materialSize = sizeof(Material);
		materialSize = (materialSize + 255) & ~255;
		meshRes.materialResource = CreateBufferResource(device, materialSize);
		meshRes.materialResource->Map(0, nullptr, reinterpret_cast<void**>(&meshRes.materialData));

		// MTLから読み込んだデータ（色・UV）を反映
		MaterialData& matData = modelData.materials[mesh.useMaterialName];
		meshRes.materialData->color = matData.diffuseColor;
		meshRes.materialData->enableLighting = 1;

		Matrix4x4 uvTransformMatrix = MakeScaleMatrix(matData.textureScale);
		uvTransformMatrix = Multiply(uvTransformMatrix, MakeTranslateMatrix(matData.textureOffset));
		meshRes.materialData->uvTransform = uvTransformMatrix;

		// このパーツ専用のテクスチャを読み込んでSRVを作成
		if (!matData.textureFilePath.empty()) {
			DirectX::ScratchImage mipImages = TextureManager::LoadTexture(matData.textureFilePath);
			const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
			meshRes.textureResource = TextureManager::CreateTextureResource(device, metadata);

			Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = TextureManager::UploadTextureData(meshRes.textureResource.Get(), mipImages, device, dxCommon_->GetCommandList());
			dxCommon_->FlushCommandList(); // コマンドを実行して転送を待つ

			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.Format = metadata.format;
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

			// SRVを空いている場所に作る
			D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = dxCommon_->GetCPUDescriptorHandle(srvHeap, srvSize, srvIndex);
			meshRes.textureHandleGPU = dxCommon_->GetGPUDescriptorHandle(srvHeap, srvSize, srvIndex);
			device->CreateShaderResourceView(meshRes.textureResource.Get(), &srvDesc, textureSrvHandleCPU);

			srvIndex++; // 次のテクスチャが来たら被らないように+1する
		}

		// 完成したパーツを配列に追加！
		meshResources_.push_back(meshRes);
	}


	// ==========================================
	//  全体で共有するリソース（ライト、WVP行列）
	// ==========================================
	uint32_t lightSize = sizeof(DirectionalLight);
	lightSize = (lightSize + 255) & ~255;
	directionalLightResource_ = CreateBufferResource(device, lightSize);
	directionalLightResource_->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData_));
	directionalLightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightData_->direction = { 0.0f, -1.0f, 0.0f };
	directionalLightData_->intensity = 1.0f;

	uint32_t transformMatrixSize = sizeof(TransformationMatrix);
	transformMatrixSize = (transformMatrixSize + 255) & ~255;
	wvpResource_ = CreateBufferResource(device, transformMatrixSize);
	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
	wvpData_->WVP = MakeIdentity4x4();
	wvpData_->World = MakeIdentity4x4();
}

// ==========================================
// 更新
// ==========================================
void Object3d::Update() {
	transform_.rotate.y += 0.03f;
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform_.scale, cameraTransform_.rotate, cameraTransform_.translate);
	Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix_, projectionMatrix_));
	

	wvpData_->WVP = worldViewProjectionMatrix;
	wvpData_->World = worldMatrix;

#ifdef USE_IMGUI
	ImGui::Begin("Settings");
	if (ImGui::TreeNode("Camera")) {
		ImGui::DragFloat3("CameraTranslate", &cameraTransform_.translate.x, 0.01f);
		ImGui::DragFloat("CameraRotateX", &cameraTransform_.rotate.x, 0.01f, 0.0f, 0.0f, "%.3f deg");
		ImGui::DragFloat("CameraRotateY", &cameraTransform_.rotate.y, 0.01f, 0.0f, 0.0f, "%.3f deg");
		ImGui::DragFloat("CameraRotateZ", &cameraTransform_.rotate.z, 0.01f, 0.0f, 0.0f, "%.3f deg");
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Directional Light")) {
		ImGui::ColorEdit4("LightColor", &directionalLightData_->color.x);
		if (ImGui::DragFloat3("LightDirection", &directionalLightData_->direction.x, 0.01f, -1.0f, 1.0f)) {
			float len = std::sqrt(directionalLightData_->direction.x * directionalLightData_->direction.x + directionalLightData_->direction.y * directionalLightData_->direction.y + directionalLightData_->direction.z * directionalLightData_->direction.z);
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
#endif
	
}

// ==========================================
// 描画
// ==========================================
void Object3d::Draw() {
	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
	D3D12_VIEWPORT viewport{};
	viewport.Width = WindowApp::kClientWidth;
	viewport.Height = WindowApp::kClientHeight;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	D3D12_RECT scissorRect{};
	scissorRect.left = 0;
	scissorRect.right = WindowApp::kClientWidth;
	scissorRect.top = 0;
	scissorRect.bottom = WindowApp::kClientHeight;

	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
	commandList->SetGraphicsRootSignature(rootSignature_.Get());
	commandList->SetPipelineState(graphicsPipelineState_.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D12DescriptorHeap* descriptorHeaps[] = { dxCommon_->GetSrvDescriptorHeap() };
	commandList->SetDescriptorHeaps(1, descriptorHeaps);

	// 全体で共有する WVP と ライト は先にセットしておく
	commandList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource_->GetGPUVirtualAddress());

	// ==========================================
	// パーツの数だけループして描画する！
	// ==========================================
	for (const auto& meshRes : meshResources_) {
		// そのパーツの頂点データ
		commandList->IASetVertexBuffers(0, 1, &meshRes.vertexBufferView);

		// そのパーツのマテリアルデータ（色やUV）
		commandList->SetGraphicsRootConstantBufferView(0, meshRes.materialResource->GetGPUVirtualAddress());

		// そのパーツのテクスチャ
		commandList->SetGraphicsRootDescriptorTable(2, meshRes.textureHandleGPU);

		// 描画
		commandList->DrawInstanced(meshRes.vertexCount, 1, 0, 0);
	}
}

// ==========================================
// バッファ作成用ヘルパー
// ==========================================
Microsoft::WRL::ComPtr<ID3D12Resource> Object3d::CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {
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
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
	assert(SUCCEEDED(hr));
	return resource;
}

