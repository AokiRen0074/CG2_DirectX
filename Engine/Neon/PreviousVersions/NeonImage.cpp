#include "NeonImage.h"
#include <cassert>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Microsoft::WRL;
using namespace DirectX;

void NeonImage::Initialize(DirectXCommon* dxCommon, const wchar_t* filePath) {
	dxCommon_ = dxCommon; 

	ID3D12Device* device = dxCommon_->GetDevice();
	HRESULT hr;

	// ==========================================
	// 1. DirectXTex を使った画像の読み込み
	// ==========================================
	TexMetadata metadata = {};
	ScratchImage scratchImg = {};
	hr = LoadFromWICFile(filePath, WIC_FLAGS_NONE, &metadata, scratchImg);
	assert(SUCCEEDED(hr) && "画像の読み込みに失敗しました！ファイルパスを確認してください。");

	const Image* img = scratchImg.GetImage(0, 0, 0);

	// ==========================================
	// 2. テクスチャリソースの作成とデータ転送
	// ==========================================
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = (UINT)metadata.width;
	texDesc.Height = (UINT)metadata.height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = metadata.format;
	texDesc.SampleDesc.Count = 1;

	D3D12_HEAP_PROPERTIES heapPropsTex = {D3D12_HEAP_TYPE_CUSTOM};
	heapPropsTex.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
	heapPropsTex.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

	hr = device->CreateCommittedResource(&heapPropsTex, D3D12_HEAP_FLAG_NONE, &texDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr, IID_PPV_ARGS(&textureResource_));
	assert(SUCCEEDED(hr));

	hr = textureResource_->WriteToSubresource(0, nullptr, img->pixels, (UINT)img->rowPitch, (UINT)img->slicePitch);
	assert(SUCCEEDED(hr));

	// SRV(テクスチャの箱)の作成
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	hr = device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&srvHeap_));
	assert(SUCCEEDED(hr));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = texDesc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(textureResource_.Get(), &srvDesc, srvHeap_->GetCPUDescriptorHandleForHeapStart());

	// ==========================================
	// 3. 頂点バッファとインデックスバッファの作成
	// ==========================================
	Vertex vertices[] = {
	    {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}}, // 左下
	    {{-1.0f, 1.0f, 0.0f},  {0.0f, 0.0f}}, // 左上
	    {{1.0f, -1.0f, 0.0f},  {1.0f, 1.0f}}, // 右下
	    {{1.0f, 1.0f, 0.0f},   {1.0f, 0.0f}}, // 右上
	};
	uint16_t indices[] = {0, 1, 2, 1, 3, 2};

	D3D12_HEAP_PROPERTIES uploadHeap = {D3D12_HEAP_TYPE_UPLOAD};
	D3D12_RESOURCE_DESC bufDesc = {};
	bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufDesc.Height = 1;
	bufDesc.DepthOrArraySize = 1;
	bufDesc.MipLevels = 1;
	bufDesc.SampleDesc.Count = 1;
	bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// 頂点
	bufDesc.Width = sizeof(vertices);
	hr = device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer_));
	assert(SUCCEEDED(hr));
	void* pV;
	vertexBuffer_->Map(0, nullptr, &pV);
	memcpy(pV, vertices, sizeof(vertices));
	vertexBuffer_->Unmap(0, nullptr);
	vbView_ = {vertexBuffer_->GetGPUVirtualAddress(), (UINT)sizeof(vertices), (UINT)sizeof(Vertex)};

	// インデックス
	bufDesc.Width = sizeof(indices);
	hr = device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBuffer_));
	assert(SUCCEEDED(hr));
	void* pI;
	indexBuffer_->Map(0, nullptr, &pI);
	memcpy(pI, indices, sizeof(indices));
	indexBuffer_->Unmap(0, nullptr);
	ibView_ = {indexBuffer_->GetGPUVirtualAddress(), (UINT)sizeof(indices), DXGI_FORMAT_R16_UINT};

	// ==========================================
	// 4. 定数バッファの作成
	// ==========================================
	bufDesc.Width = (sizeof(TransformData) + 255) & ~255;
	hr = device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&cbTransform_));
	assert(SUCCEEDED(hr));
	cbTransform_->Map(0, nullptr, (void**)&mappedTransform_);

	bufDesc.Width = (sizeof(LuminanceSettings) + 255) & ~255;
	hr = device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&cbLuminance_));
	assert(SUCCEEDED(hr));
	cbLuminance_->Map(0, nullptr, (void**)&mappedLuminance_);

	// ==========================================
	// 5. ルートシグネチャの作成
	// ==========================================
	D3D12_DESCRIPTOR_RANGE descRange = {};
	descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descRange.NumDescriptors = 1;
	descRange.BaseShaderRegister = 0; // t0
	descRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParams[3] = {};
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[0].Descriptor.ShaderRegister = 0; // b0
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[1].Descriptor.ShaderRegister = 1; // b1
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[2].DescriptorTable.pDescriptorRanges = &descRange;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.ShaderRegister = 0; // s0
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rsDesc = {};
	rsDesc.NumParameters = 3;
	rsDesc.pParameters = rootParams;
	rsDesc.NumStaticSamplers = 1;
	rsDesc.pStaticSamplers = &samplerDesc;
	rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ComPtr<ID3DBlob> rsBlob, errBlob;
	hr = D3D12SerializeRootSignature(&rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rsBlob, &errBlob);
	assert(SUCCEEDED(hr));
	hr = device->CreateRootSignature(0, rsBlob->GetBufferPointer(), rsBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));

	// ==========================================
	// 6. シェーダーの読み込み
	// ==========================================
	ComPtr<ID3DBlob> vsBlob, psNormalBlob, psLuminanceBlob;
	// 頂点シェーダーは既存のものを再利用
	hr = D3DReadFileToBlob(L"NeonObject.VS.cso", &vsBlob);
	assert(SUCCEEDED(hr) && "NeonObject.VS.cso が見つかりません！");

	// 新しく作った2つのピクセルシェーダー
	hr = D3DReadFileToBlob(L"Texture.PS.cso", &psNormalBlob);
	assert(SUCCEEDED(hr) && "Texture.PS.cso が見つかりません！");

	hr = D3DReadFileToBlob(L"Luminance.PS.cso", &psLuminanceBlob);
	assert(SUCCEEDED(hr) && "Luminance.PS.cso が見つかりません！");

	// ==========================================
	// 7. パイプラインステート(PSO)の作成
	// ==========================================
	D3D12_INPUT_ELEMENT_DESC layout[] = {
	    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = rootSignature_.Get();
	psoDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
	psoDesc.InputLayout = {layout, _countof(layout)};

	// RasterizerState の安全な初期化
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.RasterizerState.DepthClipEnable = TRUE;

	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	psoDesc.SampleDesc.Count = 1;

	// ★修正ポイント2：BlendStateをサボらずにすべて厳密に書く！
	// (DX12では0は無効な値なので、アルファ計算も全部書かないとクラッシュします)
	D3D12_RENDER_TARGET_BLEND_DESC blendDesc = {};
	blendDesc.BlendEnable = TRUE;
	blendDesc.LogicOpEnable = FALSE;
	blendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;   // ←ここをサボると死ぬ！
	blendDesc.DestBlendAlpha = D3D12_BLEND_ZERO; // ←ここをサボると死ぬ！
	blendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD; // ←ここをサボると死ぬ！
	blendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	psoDesc.BlendState.RenderTarget[0] = blendDesc;

	// 【通常描画用PSO】
	psoDesc.PS = {psNormalBlob->GetBufferPointer(), psNormalBlob->GetBufferSize()};
	// ★修正ポイント1：_SRGBを外して、エンジン標準フォーマットに合わせる！
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateNormal_));
	assert(SUCCEEDED(hr) && "通常描画用PSOの作成に失敗！");

	// 【輝度抽出用PSO】
	// 加算合成用のBlendStateを新しく作ってセットする
	D3D12_RENDER_TARGET_BLEND_DESC addBlendDesc = blendDesc;
	addBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	addBlendDesc.DestBlend = D3D12_BLEND_ONE; // 光を重ねる加算合成
	psoDesc.BlendState.RenderTarget[0] = addBlendDesc;

	psoDesc.PS = {psLuminanceBlob->GetBufferPointer(), psLuminanceBlob->GetBufferSize()};
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT; // HDRキャンバス用

	hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineStateLuminance_));
	assert(SUCCEEDED(hr) && "輝度抽出用PSOの作成に失敗！");

	aspectRatio_ = (float)metadata.width / (float)metadata.height;
}

void NeonImage::SetTransform(XMFLOAT3 pos, float baseSize, float rotZ) {
	position_ = pos;
	rotationZ_ = rotZ;
	scale_.x = baseSize * aspectRatio_;
	scale_.y = baseSize;
	scale_.z = 1.0f;
}

void NeonImage::SetLuminanceSettings(float threshold, float intensity) {
	luminanceThreshold_ = threshold;
	luminanceIntensity_ = intensity;
}

void NeonImage::Update() {

	float correctScaleX = scale_.y * aspectRatio_;

	XMMATRIX world = XMMatrixScaling(correctScaleX, scale_.y, scale_.z) * XMMatrixRotationZ(rotationZ_) * XMMatrixTranslation(position_.x, position_.y, position_.z);

	XMMATRIX view = XMMatrixLookAtLH(XMVectorSet(0, 0, -10, 1), XMVectorSet(0, 0, 0, 1), XMVectorSet(0, 1, 0, 0));
	XMMATRIX proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0f), 1280.0f / 720.0f, 0.1f, 1000.0f);
	mappedTransform_->WVP = XMMatrixTranspose(world * view * proj);

	mappedLuminance_->threshold = luminanceThreshold_;
	mappedLuminance_->intensity = luminanceIntensity_;
}

void NeonImage::Draw() {
	ID3D12GraphicsCommandList* cmdList = dxCommon_->GetCommandList();

	// 通常描画用PSOをセット
	cmdList->SetPipelineState(pipelineStateNormal_.Get());
	cmdList->SetGraphicsRootSignature(rootSignature_.Get());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &vbView_);
	cmdList->IASetIndexBuffer(&ibView_);

	// 定数バッファとテクスチャをセット
	cmdList->SetGraphicsRootConstantBufferView(0, cbTransform_->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, cbLuminance_->GetGPUVirtualAddress());

	ID3D12DescriptorHeap* ppHeaps[] = {srvHeap_.Get()};
	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	cmdList->SetGraphicsRootDescriptorTable(2, srvHeap_->GetGPUDescriptorHandleForHeapStart());

	cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void NeonImage::DrawLuminance() {
	ID3D12GraphicsCommandList* cmdList = dxCommon_->GetCommandList();

	// 輝度抽出用PSOをセット（ここ以外はDraw()と全く同じ）
	cmdList->SetPipelineState(pipelineStateLuminance_.Get());
	cmdList->SetGraphicsRootSignature(rootSignature_.Get());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &vbView_);
	cmdList->IASetIndexBuffer(&ibView_);

	cmdList->SetGraphicsRootConstantBufferView(0, cbTransform_->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, cbLuminance_->GetGPUVirtualAddress());

	ID3D12DescriptorHeap* ppHeaps[] = {srvHeap_.Get()};
	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
	cmdList->SetGraphicsRootDescriptorTable(2, srvHeap_->GetGPUDescriptorHandleForHeapStart());

	cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void NeonImage::DrawImGui(const std::string& label) {
	// 【重要】PushIDを使うことで、複数の画像で同じ「しきい値」という名前のスライダーを使っても内部で混線しなくなります
	ImGui::PushID(label.c_str());

	ImGui::Text("【 %s 】", label.c_str()); // 看板の名前を表示

	// スライダーで直接メンバー変数を書き換える！
	// 書式：DragFloat("画面上の名前", &変数, 変化スピード, 最小値, 最大値)
	ImGui::DragFloat("しきい値 (Threshold)", &luminanceThreshold_, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("発光強度 (Intensity)", &luminanceIntensity_, 0.1f, 0.0f, 50.0f);

	// 💡ついでに位置とサイズもいじれるようにしておくと、配置作業が神レベルで楽になります！
	ImGui::DragFloat3("位置 (Position)", &position_.x, 0.05f);
	ImGui::DragFloat3("サイズ (Scale)", &scale_.x, 0.05f);

	ImGui::Separator(); // 区切り線
	ImGui::PopID();
}