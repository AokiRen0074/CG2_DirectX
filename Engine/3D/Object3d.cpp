#include "Object3d.h"
#include <cassert>
#include "DirectXCommon.h" 
#include "WindowApp.h"
#include "TextureManager.h"
#include <numbers>
#include "Model.h"
#include "ModelManager.h"
#include <stdexcept>
#include <cmath>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#endif

DirectXCommon* Object3d::sDxCommon_ = nullptr;


void Object3d::StaticInitialize(DirectXCommon* dxCommon) {
	sDxCommon_ = dxCommon;
}

//Create関数
Object3d* Object3d::Create(const std::string& directoryPath, const std::string& filename) {
	// メモリを確保
	auto object3d = std::make_unique<Object3d>();

	// 初期化処理
	object3d->Initialize(directoryPath, filename);

	return object3d.release();
}

/*--------------------------
初期化
-----------------------------------*/
void Object3d::Initialize(const std::string& directoryPath, const std::string& filename) {

	dxCommon_ = sDxCommon_;

	if (!dxCommon_) {
		throw std::logic_error("Object3d requires StaticInitialize");
	}
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

	// RasterizerStateの設定
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	// Shaderをコンパイルする
	Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = dxCommon_->CompilerShader(L"Resources/Shaders/Object3D.VS.hlsl",
		L"vs_6_0", dxCommon_->GetDxcUtils(), dxCommon_->GetDxcCompiler(), dxCommon_->GetIncludeHandler());
	assert(vertexShaderBlob != nullptr);

	Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = dxCommon_->CompilerShader(L"Resources/Shaders/Object3D.PS.hlsl",
		L"ps_6_0", dxCommon_->GetDxcUtils(), dxCommon_->GetDxcCompiler(), dxCommon_->GetIncludeHandler());
	assert(pixelShaderBlob != nullptr);

	// DepthStencilStateの設定
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rootSignature_.Get();
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize() };
	graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize() };
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
	graphicsPipelineStateDesc.NumRenderTargets = 1;
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	graphicsPipelineStateDesc.SampleDesc.Count = 1;
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;


	for (int i = 0; i < static_cast<int>(BlendMode::kCount); i++) {
		// BlendStateの設定
		D3D12_BLEND_DESC blendDesc{};
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

		switch (static_cast<BlendMode>(i)) {
		case BlendMode::kNormal:
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			break;
		case BlendMode::kAdd:
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			break;
		case BlendMode::kSubtract:
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			break;
		case BlendMode::kMultiply:
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ZERO;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_COLOR;
			break;
		case BlendMode::kScreen:
			blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
			blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
			break;
		}

		graphicsPipelineStateDesc.BlendState = blendDesc;

		// 配列の [i] 番目に生成
		hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineStates_[i]));
		assert(SUCCEEDED(hr));
	}


	D3D12_ROOT_PARAMETER rootParamsNoTex[3] = {};
	rootParamsNoTex[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParamsNoTex[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParamsNoTex[0].Descriptor.ShaderRegister = 0; // マテリアル(b0)

	rootParamsNoTex[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParamsNoTex[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParamsNoTex[1].Descriptor.ShaderRegister = 0; // WVP(b0)

	rootParamsNoTex[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParamsNoTex[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParamsNoTex[2].Descriptor.ShaderRegister = 1; // ライト

	D3D12_ROOT_SIGNATURE_DESC rootSigDescNoTex{};
	rootSigDescNoTex.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSigDescNoTex.pParameters = rootParamsNoTex;
	rootSigDescNoTex.NumParameters = _countof(rootParamsNoTex);

	ID3DBlob* sigBlobNoTex = nullptr;
	ID3DBlob* errBlobNoTex = nullptr;
	hr = D3D12SerializeRootSignature(&rootSigDescNoTex, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlobNoTex, &errBlobNoTex);
	assert(SUCCEEDED(hr));
	hr = device->CreateRootSignature(0, sigBlobNoTex->GetBufferPointer(), sigBlobNoTex->GetBufferSize(), IID_PPV_ARGS(&rootSignatureNoTexture_));
	assert(SUCCEEDED(hr));

	// 新しいShaderのコンパイル
	Microsoft::WRL::ComPtr<IDxcBlob> vsBlobNoTex = dxCommon_->CompilerShader(L"Resources/Shaders/Object3D.NoTexture.VS.hlsl", L"vs_6_0", dxCommon_->GetDxcUtils(), dxCommon_->GetDxcCompiler(), dxCommon_->GetIncludeHandler());
	Microsoft::WRL::ComPtr<IDxcBlob> psBlobNoTex = dxCommon_->CompilerShader(L"Resources/Shaders/Object3D.NoTexture.PS.hlsl", L"ps_6_0", dxCommon_->GetDxcUtils(), dxCommon_->GetDxcCompiler(), dxCommon_->GetIncludeHandler());

	// パイプラインの作成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDescNoTex = graphicsPipelineStateDesc;
	psoDescNoTex.pRootSignature = rootSignatureNoTexture_.Get();
	psoDescNoTex.VS = { vsBlobNoTex->GetBufferPointer(), vsBlobNoTex->GetBufferSize() };
	psoDescNoTex.PS = { psBlobNoTex->GetBufferPointer(), psBlobNoTex->GetBufferSize() };

	D3D12_BLEND_DESC normalBlendDesc{};
	normalBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	normalBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	normalBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	normalBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	normalBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	normalBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	normalBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	normalBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;

	psoDescNoTex.BlendState = normalBlendDesc;

	hr = device->CreateGraphicsPipelineState(&psoDescNoTex, IID_PPV_ARGS(&graphicsPipelineStateNoTexture_));
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




	// ==========================================
	// OBJモデルデータの読み込み
	// ==========================================

	SetModel(directoryPath, filename);
	defaultTextureHandle_ = TextureManager::Load("Resources/white1x1.png");

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
	directionalLightData_->lightingType = 1;
	directionalLightData_->scene = {};

	uint32_t transformMatrixSize = sizeof(TransformationMatrix);
	transformMatrixSize = (transformMatrixSize + 255) & ~255;
	wvpResource_ = CreateBufferResource(device, transformMatrixSize);
	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
	wvpData_->WVP = MakeIdentity4x4();
	wvpData_->World = MakeIdentity4x4();

	UpdateMatrices(MakeIdentity4x4());
}

// モデルの切り替え
void Object3d::SetModel(const std::string& directoryPath, const std::string& filename) {
	SetModel(ModelManager::Load(directoryPath, filename));
}

void Object3d::SetModel(const std::shared_ptr<const ModelData>& model) {
	if (!dxCommon_ || !model) {
		throw std::invalid_argument("SetModel requires an initialized object and model");
	}

	if (model_ == model) {
		return;
	}

	// 読み込み済みの頂点バッファを使い、画像やモデルを読み直さない
	auto sharedVertices = ModelManager::GetVertexResources(dxCommon_->GetDevice(), model);
	const ModelData& modelData = *model;
	std::vector<MeshResource> newMeshResources;
	newMeshResources.reserve(modelData.meshes.size());
	ID3D12Device* device = dxCommon_->GetDevice();


	// ==========================================
	//  メッシュの数だけバッファを作るループ
	// ==========================================
	for (size_t i = 0; i < modelData.meshes.size(); ++i) {
		const auto& mesh = modelData.meshes[i];
		const auto& sharedMesh = sharedVertices->meshes[i];

		if (sharedMesh.vertexCount == 0) {
			continue;
		}

		MeshResource meshRes;
		meshRes.vertexCount = sharedMesh.vertexCount;

		// 頂点バッファの作成
		// 作成済みのバッファを共有し、オブジェクトごとの重複を避ける
		meshRes.vertexResource = sharedMesh.vertexResource;
		meshRes.vertexBufferView = sharedMesh.vertexBufferView;

		// マテリアルバッファの作成
		uint32_t materialSize = sizeof(Material);
		materialSize = (materialSize + 255) & ~255;
		meshRes.materialResource = CreateBufferResource(device, materialSize);
		HRESULT hr = meshRes.materialResource->Map(
			0, nullptr, reinterpret_cast<void**>(&meshRes.materialData));

		if (FAILED(hr)) {
			throw std::runtime_error("Failed to map model material buffer");
		}

		*meshRes.materialData = {};

		// MTLから読み込んだデータ（色・UV）を反映
		const auto material = modelData.materials.find(mesh.useMaterialName);
		const MaterialData defaultMaterial;
		const MaterialData& matData = material != modelData.materials.end()
			? material->second : defaultMaterial;
		meshRes.materialData->color = matData.diffuseColor;
		meshRes.materialData->enableLighting = 1;

		meshRes.uvTransform.scale = matData.textureScale;
		meshRes.uvTransform.translate = matData.textureOffset;
		meshRes.uvTransform.rotate = { 0.0f, 0.0f, 0.0f }; // 初期回転はゼロ

		// SRTからAffine行列を作ってGPUに送る
		meshRes.materialData->uvTransform = MakeAffineMatrix(
			meshRes.uvTransform.scale,
			meshRes.uvTransform.rotate,
			meshRes.uvTransform.translate
		);

		// このパーツ専用のテクスチャを読み込んでSRVを作成
		if (!matData.textureFilePath.empty() && matData.textureFilePath.find(".") != std::string::npos) {
			// TextureManager に安全な場所へロードしてもらい、そのハンドルをもらう
			uint32_t handle = TextureManager::Load(matData.textureFilePath);
			meshRes.textureHandleGPU = TextureManager::GetInstance()->GetSrvHandleGPU(handle);
		}
		else {
			// 画像がない場合はダミーを入れてクラッシュを防ぐ
			meshRes.textureHandleGPU = TextureManager::GetInstance()->GetSrvHandleGPU(0);
		}

		// 完成したパーツを配列に追加！
		newMeshResources.push_back(std::move(meshRes));
	}

	// 全パーツの準備ができてから切り替え、途中で失敗した場合は元のモデルを残す
	meshResources_ = std::move(newMeshResources);
	model_ = model;
}

void Object3d::SetPosition(const Vector3& position) {
	transform_.translate = position;
	UpdateMatrices(MakeIdentity4x4());
}

void Object3d::SetScale(const Vector3& scale) {
	transform_.scale = scale;
	UpdateMatrices(MakeIdentity4x4());
}

void Object3d::SetRotation(const Vector3& rotation) {
	transform_.rotate = rotation;
	UpdateMatrices(MakeIdentity4x4());
}

void Object3d::UpdateMatrices(const Matrix4x4& parentMatrix) {
	worldMatrix_ = MakeAffineMatrix(transform_.scale, transform_.rotate, transform_.translate);
	worldMatrix_ = Multiply(worldMatrix_, parentMatrix);

	if (wvpData_) {
		wvpData_->World = worldMatrix_;
		wvpData_->WVP = Multiply(worldMatrix_, Multiply(viewMatrix_, projectionMatrix_));
	}
}

// ==========================================
// 更新
// ==========================================
void Object3d::Update() {
	//transform_.rotate.y += 0.03f;
	UpdateMatrices(MakeIdentity4x4());

#ifdef USE_IMGUI
	ImGui::Begin("Settings");
	ImGui::PushID(this);

	const char* blendModeNames[] = { "Normal", "Add", "Subtract", "Multiply", "Screen" };
	ImGui::Combo("Blend Mode", &blendMode_, blendModeNames, IM_ARRAYSIZE(blendModeNames));

	ImGui::DragFloat3("CameraTranslate", &cameraTransform_.translate.x, 0.01f);
	ImGui::DragFloat("CameraRotateX", &cameraTransform_.rotate.x, 0.01f, 0.0f, 0.0f, "%.3f deg");
	ImGui::DragFloat("CameraRotateY", &cameraTransform_.rotate.y, 0.01f, 0.0f, 0.0f, "%.3f deg");
	ImGui::DragFloat("CameraRotateZ", &cameraTransform_.rotate.z, 0.01f, 0.0f, 0.0f, "%.3f deg");

	ImGui::DragFloat("SphereRotateX", &transform_.rotate.x, 0.01f, 0.0f, 0.0f, "%.3f deg");
	ImGui::DragFloat("SphereRotateY", &transform_.rotate.y, 0.01f, 0.0f, 0.0f, "%.3f deg");
	ImGui::DragFloat("SphereRotateZ", &transform_.rotate.z, 0.01f, 0.0f, 0.0f, "%.3f deg");

	ImGui::ColorEdit3("LightColor", &directionalLightData_->color.x);
	ImGui::DragFloat3("LightDirection", &directionalLightData_->direction.x, 0.01f, -1.0f, 1.0f);
	ImGui::DragFloat("Intensity", &directionalLightData_->intensity, 0.01f);

	ImGui::Checkbox("useMonsterBall", &useMonsterBall_);

	static float colorSprite[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	ImGui::ColorEdit4("colorSprite", colorSprite);
	static float translateSprite[3] = { 0.0f, 0.0f, 0.0f };
	ImGui::DragFloat3("translateSprite", translateSprite, 0.01f);


	for (size_t i = 0; i < meshResources_.size(); ++i) {
		ImGui::PushID(static_cast<int>(i));

		std::string nodeName = "Mesh " + std::to_string(i);
		if (ImGui::TreeNode(nodeName.c_str())) {
			ImGui::ColorEdit4("color", &meshResources_[i].materialData->color.x);

			bool isLighting = meshResources_[i].materialData->enableLighting != 0;
			if (ImGui::Checkbox("enableLighting", &isLighting)) {
				meshResources_[i].materialData->enableLighting = isLighting ? 1 : 0;
			}

			ImGui::DragFloat2("UVTranslate", &meshResources_[i].uvTransform.translate.x, 0.01f);
			ImGui::DragFloat2("UVScale", &meshResources_[i].uvTransform.scale.x, 0.01f);
			ImGui::DragFloat("UVRotate", &meshResources_[i].uvTransform.rotate.z, 0.01f, 0.0f, 0.0f, "%.3f deg");

			ImGui::TreePop(); // ツリーを閉じる
		}

		ImGui::PopID();

		meshResources_[i].materialData->uvTransform = MakeAffineMatrix(
			meshResources_[i].uvTransform.scale,
			meshResources_[i].uvTransform.rotate,
			meshResources_[i].uvTransform.translate
		);
	}

	ImGui::PopID();
	ImGui::End();
#endif
}

// ==========================================
// 描画
// ==========================================

// オブジェクト自身のワールド行列を使う描画
void Object3d::Draw(const ViewProjection& viewProjection, uint32_t textureHandle) {
	WorldTransform parentTransform;
	Draw(parentTransform, viewProjection, textureHandle);
}

void Object3d::Draw(const ViewProjection& viewProjection) {
	WorldTransform parentTransform;
	Draw(parentTransform, viewProjection);
}

void Object3d::Draw(uint32_t textureHandle) {
	ViewProjection viewProjection;
	viewProjection.matView = viewMatrix_;
	viewProjection.matProjection = projectionMatrix_;
	Draw(viewProjection, textureHandle);
}

void Object3d::Draw(const WorldTransform& worldTransform, const ViewProjection& viewProjection, uint32_t textureHandle) {
	// 渡されたカメラと親の行列を、オブジェクト自身の行列に反映する
	SetCameraMatrix(viewProjection.matView, viewProjection.matProjection);
	UpdateMatrices(worldTransform.matWorld_);

	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
	D3D12_VIEWPORT viewport{};
	viewport.Width = WindowApp::kClientWidth;
	viewport.Height = WindowApp::kClientHeight;
	viewport.TopLeftX = 0; viewport.TopLeftY = 0; viewport.MinDepth = 0.0f; viewport.MaxDepth = 1.0f;
	D3D12_RECT scissorRect{};
	scissorRect.left = 0; scissorRect.right = WindowApp::kClientWidth; scissorRect.top = 0; scissorRect.bottom = WindowApp::kClientHeight;

	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D12DescriptorHeap* descriptorHeaps[] = { dxCommon_->GetSrvDescriptorHeap() };
	commandList->SetDescriptorHeaps(1, descriptorHeaps);

	for (const auto& meshRes : meshResources_) {
		commandList->IASetVertexBuffers(0, 1, &meshRes.vertexBufferView);

		commandList->SetGraphicsRootSignature(rootSignature_.Get());
		commandList->SetPipelineState(pipelineStates_[blendMode_].Get());

		commandList->SetGraphicsRootConstantBufferView(0, meshRes.materialResource->GetGPUVirtualAddress());
		commandList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());

		if (meshRes.textureHandleGPU.ptr != 0) {
			commandList->SetGraphicsRootDescriptorTable(2, meshRes.textureHandleGPU);
		}
		else {
			D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = TextureManager::GetInstance()->GetSrvHandleGPU(textureHandle != 0 ? textureHandle : defaultTextureHandle_);
			commandList->SetGraphicsRootDescriptorTable(2, gpuHandle);
		}
		commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource_->GetGPUVirtualAddress());

		commandList->DrawInstanced(meshRes.vertexCount, 1, 0, 0);
	}
}

// テクスチャなし
void Object3d::Draw(const WorldTransform& worldTransform, const ViewProjection& viewProjection) {
	// 渡されたカメラと親の行列を、オブジェクト自身の行列に反映する
	SetCameraMatrix(viewProjection.matView, viewProjection.matProjection);
	UpdateMatrices(worldTransform.matWorld_);

	ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
	D3D12_VIEWPORT viewport{};
	viewport.Width = WindowApp::kClientWidth;
	viewport.Height = WindowApp::kClientHeight;
	viewport.TopLeftX = 0; viewport.TopLeftY = 0; viewport.MinDepth = 0.0f; viewport.MaxDepth = 1.0f;
	D3D12_RECT scissorRect{};
	scissorRect.left = 0; scissorRect.right = WindowApp::kClientWidth; scissorRect.top = 0; scissorRect.bottom = WindowApp::kClientHeight;

	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 後続のスプライト描画のためにヒープだけはセットしておく
	ID3D12DescriptorHeap* descriptorHeaps[] = { dxCommon_->GetSrvDescriptorHeap() };
	commandList->SetDescriptorHeaps(1, descriptorHeaps);

	for (const auto& meshRes : meshResources_) {
		commandList->IASetVertexBuffers(0, 1, &meshRes.vertexBufferView);

		// テクスチャなし専用のパイプライン！
		commandList->SetGraphicsRootSignature(rootSignatureNoTexture_.Get());
		commandList->SetPipelineState(graphicsPipelineStateNoTexture_.Get());

		commandList->SetGraphicsRootConstantBufferView(0, meshRes.materialResource->GetGPUVirtualAddress());
		commandList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
		commandList->SetGraphicsRootConstantBufferView(2, directionalLightResource_->GetGPUVirtualAddress()); // ライトは2番

		commandList->DrawInstanced(meshRes.vertexCount, 1, 0, 0);
	}

	// スプライト描画のクラッシュを防ぐ
	commandList->SetGraphicsRootSignature(rootSignature_.Get());
	commandList->SetPipelineState(pipelineStates_[0].Get());
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

void Object3d::SetDirectionalLight(const Vector3& direction, const Vector3& color, float intensity) {
    const float length = std::sqrt(direction.x * direction.x + direction.y * direction.y
        + direction.z * direction.z);

    if (length > 0.0001f) {
        directionalLightData_->direction = { direction.x / length, direction.y / length,
            direction.z / length };
    }
    else {
        directionalLightData_->direction = { -0.57735f, -0.57735f, 0.57735f };
    }

    directionalLightData_->color = { color.x, color.y, color.z, 1.0f };
    directionalLightData_->intensity = intensity;

	// 槍ステージではハーフランバートで暗い面の輪郭も読めるようにする。
	directionalLightData_->lightingType = 2;
}

void Object3d::SetSceneLighting(const SceneLighting& lighting) {
    directionalLightData_->scene = lighting;
}
