#include "NeonPlayer.h"
#include <cassert>
#include <cmath> 
#include <d3dcompiler.h>
#include "Input.h"
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif


using namespace Microsoft::WRL;


void NeonPlayer::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	ID3D12Device* device = dxCommon_->GetDevice();
	HRESULT hr;

	//インデックスバッファ作成 ---
	Vertex vertices[] = {
	    {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
	    {{-1.0f, 1.0f, 0.0f},  {0.0f, 0.0f}},
	    {{1.0f, -1.0f, 0.0f},  {1.0f, 1.0f}},
	    {{1.0f, 1.0f, 0.0f},   {1.0f, 0.0f}},
	};
	uint16_t indices[] = {0, 1, 2, 1, 3, 2};

	D3D12_HEAP_PROPERTIES uploadHeap = {D3D12_HEAP_TYPE_UPLOAD};
	D3D12_RESOURCE_DESC bufDesc = {};
	bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufDesc.Width = sizeof(vertices);
	bufDesc.Height = 1;
	bufDesc.DepthOrArraySize = 1;
	bufDesc.MipLevels = 1;
	bufDesc.SampleDesc.Count = 1;
	bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	hr = device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vertexBuffer_));
	assert(SUCCEEDED(hr));
	void* pV;
	vertexBuffer_->Map(0, nullptr, &pV);
	memcpy(pV, vertices, sizeof(vertices));
	vertexBuffer_->Unmap(0, nullptr);
	vbView_ = {vertexBuffer_->GetGPUVirtualAddress(), (UINT)sizeof(vertices), (UINT)sizeof(Vertex)};

	bufDesc.Width = sizeof(indices);
	hr = device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&indexBuffer_));
	assert(SUCCEEDED(hr));
	void* pI;
	indexBuffer_->Map(0, nullptr, &pI);
	memcpy(pI, indices, sizeof(indices));
	indexBuffer_->Unmap(0, nullptr);
	ibView_ = {indexBuffer_->GetGPUVirtualAddress(), (UINT)sizeof(indices), DXGI_FORMAT_R16_UINT};

	// --- 2. 定数バッファ作成 ---
	bufDesc.Width = (sizeof(TransformData) + 255) & ~255;
	device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&cbTransform_));
	cbTransform_->Map(0, nullptr, (void**)&mappedTransform_);

	bufDesc.Width = (sizeof(ColorData) + 255) & ~255;
	device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &bufDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&cbColor_));
	cbColor_->Map(0, nullptr, (void**)&mappedColor_);

	// --- 3. ルートシグネチャ作成 ---
	D3D12_ROOT_PARAMETER rp[2] = {};
	rp[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rp[0].Descriptor.ShaderRegister = 0; // b0
	rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rp[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rp[1].Descriptor.ShaderRegister = 1; // b1
	rp[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rsD = {2, rp, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};
	ComPtr<ID3DBlob> rsBlob, errBlob;
	D3D12SerializeRootSignature(&rsD, D3D_ROOT_SIGNATURE_VERSION_1, &rsBlob, &errBlob);
	device->CreateRootSignature(0, rsBlob->GetBufferPointer(), rsBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));

	// --- 4. シェーダー読み込み ---
	ComPtr<ID3DBlob> vsBlob, psBlob;
	if (FAILED(D3DReadFileToBlob(L"NeonObject.VS.cso", &vsBlob)) || FAILED(D3DReadFileToBlob(L"NeonPlayer.PS.cso", &psBlob))) {
		assert(false);
	}

	// --- 5. パイプラインステート(PSO)作成 ---
	D3D12_INPUT_ELEMENT_DESC layout[] = {
	    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = rootSignature_.Get();
	psoDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
	psoDesc.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()};
	psoDesc.InputLayout = {layout, _countof(layout)};

	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // ネオンなので両面
	psoDesc.RasterizerState.DepthClipEnable = TRUE;

	// ブレンド設定 (加算合成)
	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	psoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // 加算
	psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

	psoDesc.DepthStencilState.DepthEnable = FALSE;
	psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT; // 形式はエンジンに合わせる

	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;

	// Bloomのテクスチャ形式
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;

	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	psoDesc.SampleDesc.Count = 1;

	hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState_));
	if (FAILED(hr)) {
		assert(false);
	}
}

// 引数に shakeX と shakeY を追加
void NeonPlayer::Update(float shakeX, float shakeY) {
	 Input* input =  Input::GetInstance();

	// ==========================================
	// 1. WASDでの移動と滑らかな傾き
	// ==========================================
	float speed = 0.1f;
	float targetRotationZ = 0.0f;

	if (input->PushKey(DIK_W)) {
		position_.y += speed;
	}
	if (input->PushKey(DIK_S)) {
		position_.y -= speed;
	}

	if (input->PushKey(DIK_A)) {
		position_.x -= speed;
		targetRotationZ = 0.25f;
	}
	if (input->PushKey(DIK_D)) {
		position_.x += speed;
		targetRotationZ = -0.25f;
	}

	rotationZ_ = rotationZ_ + (targetRotationZ - rotationZ_) * 0.15f;

	// ==========================================
	// 2. 完全自作の行列計算（平行投影 ＋ ★画面シェイク）
	// ==========================================
	auto Multiply = [](const  Matrix4x4& m1, const  Matrix4x4& m2) {
		 Matrix4x4 r = {};
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				r.m[i][j] = m1.m[i][0] * m2.m[0][j] + m1.m[i][1] * m2.m[1][j] + m1.m[i][2] * m2.m[2][j] + m1.m[i][3] * m2.m[3][j];
			}
		}
		return r;
	};

	auto Transpose = [](const  Matrix4x4& m) {
		 Matrix4x4 r = {};
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				r.m[i][j] = m.m[j][i];
			}
		}
		return r;
	};

	 Matrix4x4 world = {};
	float cosZ = std::cos(rotationZ_);
	float sinZ = std::sin(rotationZ_);
	world.m[0][0] = scale_.x * cosZ;
	world.m[0][1] = scale_.x * sinZ;
	world.m[0][2] = 0.0f;
	world.m[0][3] = 0.0f;
	world.m[1][0] = scale_.y * -sinZ;
	world.m[1][1] = scale_.y * cosZ;
	world.m[1][2] = 0.0f;
	world.m[1][3] = 0.0f;
	world.m[2][0] = 0.0f;
	world.m[2][1] = 0.0f;
	world.m[2][2] = scale_.z;
	world.m[2][3] = 0.0f;
	world.m[3][0] = position_.x;
	world.m[3][1] = position_.y;
	world.m[3][2] = position_.z;
	world.m[3][3] = 1.0f;

	 Matrix4x4 view = {};
	view.m[0][0] = 1.0f;
	view.m[1][1] = 1.0f;
	view.m[2][2] = 1.0f;
	view.m[3][3] = 1.0f;
	view.m[3][2] = 10.0f;

	// ✨ ここで自機のカメラも揺らす！
	view.m[3][0] += shakeX;
	view.m[3][1] += shakeY;

	 Matrix4x4 proj = {};
	float viewWidth = 16.0f;
	float viewHeight = 9.0f;
	float nearZ = 0.1f;
	float farZ = 1000.0f;

	proj.m[0][0] = 2.0f / viewWidth;
	proj.m[1][1] = 2.0f / viewHeight;
	proj.m[2][2] = 1.0f / (farZ - nearZ);
	proj.m[3][2] = nearZ / (nearZ - farZ);
	proj.m[3][3] = 1.0f;

	 Matrix4x4 wvp = Multiply(world, Multiply(view, proj));
	mappedTransform_->WVP = Transpose(wvp);

	// ==========================================
	// 3. ImGuiのカラーデータをGPUへ転送
	// ==========================================
	mappedColor_->color = {playerColor_[0], playerColor_[1], playerColor_[2], 1.0f};
	mappedColor_->intensity = playerIntensity_;
	mappedColor_->radius = playerRadius_;
	mappedColor_->softness = 20.0f;
	mappedColor_->length = tubeLength_;
}

void NeonPlayer::DrawImGui() {
	if (ImGui::TreeNode("Neon Player Settings")) {
		ImGui::ColorEdit3("Color", playerColor_);
		ImGui::SliderFloat("Intensity", &playerIntensity_, 0.0f, 30.0f);
		ImGui::SliderFloat("Radius", &playerRadius_, 0.001f, 0.1f);
		ImGui::TreePop();
	}
}

void NeonPlayer::Draw() {
	ID3D12GraphicsCommandList* cmdList= dxCommon_->GetCommandList();
	cmdList->SetPipelineState(pipelineState_.Get());
	cmdList->SetGraphicsRootSignature(rootSignature_.Get());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->IASetVertexBuffers(0, 1, &vbView_);
	cmdList->IASetIndexBuffer(&ibView_);
	cmdList->SetGraphicsRootConstantBufferView(0, cbTransform_->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, cbColor_->GetGPUVirtualAddress());
	cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}
