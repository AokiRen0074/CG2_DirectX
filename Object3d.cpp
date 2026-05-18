#include "Object3d.h"
#include <cassert>
#include "DirectXCommon.h" 
#include "WindowApp.h"
#include "TextureManager.h"
#include <numbers>

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
	D3D12_ROOT_PARAMETER rootParameters[3] = {};
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
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
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


	
	const uint32_t kSubdivision = 16; // 分割数
	const uint32_t kVertexCount = kSubdivision * kSubdivision * 6; // 全頂点数
	const float pi = std::numbers::pi_v<float>;

	// 頂点リソースの作成
	vertexResource_ = CreateBufferResource(device, sizeof(VertexData) * kVertexCount);

	// VBVにも新しいサイズを教える
	vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
	vertexBufferView_.SizeInBytes = sizeof(VertexData) * kVertexCount;
	vertexBufferView_.StrideInBytes = sizeof(VertexData);

	VertexData* vertexData = nullptr;
	vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));


	// 経度分割1つ分の角度
	const float kLonEvery = pi * 2.0f / float(kSubdivision);
	// 緯度分割1つ分の角度
	const float kLatEvery = pi / float(kSubdivision);

	// 緯度の方向に分割
	for (uint32_t latIndex = 0; latIndex < kSubdivision; ++latIndex) {
		float lat = -pi / 2.0f + kLatEvery * latIndex; // 現在の緯度 

		// 経度の方向に分割しながら線を描く
		for (uint32_t lonIndex = 0; lonIndex < kSubdivision; ++lonIndex) {
			uint32_t start = (latIndex * kSubdivision + lonIndex) * 6;
			float lon = lonIndex * kLonEvery; // 現在の経度


			float latCos = std::cos(lat);
			float latSin = std::sin(lat);
			float latNextCos = std::cos(lat + kLatEvery);
			float latNextSin = std::sin(lat + kLatEvery);

			float lonCos = std::cos(lon);
			float lonSin = std::sin(lon);
			float lonNextCos = std::cos(lon + kLonEvery);
			float lonNextSin = std::sin(lon + kLonEvery);

			// UV座標の計算
			float u = float(lonIndex) / float(kSubdivision);
			float v = 1.0f - float(latIndex) / float(kSubdivision);
			float uNext = float(lonIndex + 1) / float(kSubdivision);
			float vNext = 1.0f - float(latIndex + 1) / float(kSubdivision);

			// 頂点1
			vertexData[start].position = { latCos * lonCos, latSin, latCos * lonSin, 1.0f };
			vertexData[start].texcoord = { u, v };

			// 頂点2
			vertexData[start + 1].position = { latNextCos * lonCos, latNextSin, latNextCos * lonSin, 1.0f };
			vertexData[start + 1].texcoord = { u, vNext };

			// 頂点3
			vertexData[start + 2].position = { latCos * lonNextCos, latSin, latCos * lonNextSin, 1.0f };
			vertexData[start + 2].texcoord = { uNext, v };

			// 頂点4
			vertexData[start + 3].position = { latCos * lonNextCos, latSin, latCos * lonNextSin, 1.0f };
			vertexData[start + 3].texcoord = { uNext, v };

			// 頂点5
			vertexData[start + 4].position = { latNextCos * lonCos, latNextSin, latNextCos * lonSin, 1.0f };
			vertexData[start + 4].texcoord = { u, vNext };

			// 頂点6
			vertexData[start + 5].position = { latNextCos * lonNextCos, latNextSin, latNextCos * lonNextSin, 1.0f };
			vertexData[start + 5].texcoord = { uNext, vNext };
		}
	}

	// 色1つ分（Vector4）のサイズで作る
	materialResources_ = CreateBufferResource(device, sizeof(Vector4));



	// マテリアルにデータを書き込む
	materialResources_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));
	*materialData_ = materialColor_;

	// WVP用のリソースを作る。
	wvpResource_ = CreateBufferResource(device, sizeof(Matrix4x4));

	wvpResource_->Map(0, nullptr, reinterpret_cast<void**>(&wvpData_));
	*wvpData_ = MakeIdentity4x4();

	// テクスチャ読み込み処理
	DirectX::ScratchImage mipImages = TextureManager::LoadTexture("Resources/uvChecker.png");
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	textureResource_ = TextureManager::CreateTextureResource(device, metadata);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource =
		TextureManager::UploadTextureData(textureResource_.Get(), mipImages, device, dxCommon_->GetCommandList());

	dxCommon_->FlushCommandList();

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
	*wvpData_ = worldViewProjectionMatrix;

#ifdef USE_IMGUI
	ImGui::Begin("Settings");
	ImGui::ColorEdit4("Material Color", &materialColor_.x); // カラーピッカーを表示
	ImGui::End();

	*materialData_ = materialColor_;

#endif

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
	//RootSignatureを設定。PSOに設定しているけど別途設定が必要
	commandList->SetGraphicsRootSignature(rootSignature_.Get());
	commandList->SetPipelineState(graphicsPipelineState_.Get());
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
	//形状を設定。PSOに設定しているものとはまた別。同じものを設定すると考えておけば良い
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D12DescriptorHeap* descriptorHeaps[] = { dxCommon_->GetSrvDescriptorHeap() };
	commandList->SetDescriptorHeaps(1, descriptorHeaps);

	commandList->SetGraphicsRootConstantBufferView(0, materialResources_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(1, wvpResource_->GetGPUVirtualAddress());
	commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU_);

	commandList->DrawInstanced(1536, 1, 0, 0);
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