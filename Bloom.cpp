#include "Bloom.h"
#include <cassert>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

ID3D12Device* Bloom::GetDevice() { return dxCommon_->GetDevice(); }

void Bloom::Initialize(DirectXCommon* dxCommon, int windowWidth, int windowHeight) {

	dxCommon_ = dxCommon; // ポインタを保存
	ID3D12Device* device = dxCommon_->GetDevice();

	//ID3D12Device* device = GetDevice();
	assert(device != nullptr);

	// ==========================================
	// HDR用テクスチャリソースの設定
	// ==========================================
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	// ==========================================
	// リソースの設定
	// ==========================================
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Width = windowWidth;   // 画面の幅
	resourceDesc.Height = windowHeight; // 画面の高さ
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; // 1.0以上の色を保存できるHDRフォーマット
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; // 描画対象として使うフラグ

	// ==========================================
	// クリアカラーの設定
	// ==========================================
	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = resourceDesc.Format;
	clearValue.Color[0] = 0.0f; // R
	clearValue.Color[1] = 0.0f; // G
	clearValue.Color[2] = 0.0f; // B
	clearValue.Color[3] = 1.0f; // A

	// ==========================================
	// リソースの生成
	// ==========================================
	HRESULT hr = device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&clearValue, IID_PPV_ARGS(&hdrTextureResource_));
	assert(SUCCEEDED(hr));

	// ==========================================
	// 5. デスクリプタヒープの作成
	// ==========================================
	// RTVヒープ（変更なし）
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.NumDescriptors = 1;
	hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap_));
	assert(SUCCEEDED(hr));

	// SRV/UAVヒープ（★変更：格納するビューの数を「2」に増やします）
	D3D12_DESCRIPTOR_HEAP_DESC srvUavHeapDesc{};
	srvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvUavHeapDesc.NumDescriptors = 3; // SRV用に1つ、UAV用に1つ
	srvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	hr = device->CreateDescriptorHeap(&srvUavHeapDesc, IID_PPV_ARGS(&srvHeap_));
	assert(SUCCEEDED(hr));

	// ==========================================
	// 5.5. UAVリソース（書き込み用キャンバス）の生成（★新規追加）
	// ==========================================
	D3D12_RESOURCE_DESC uavDesc = resourceDesc;                 // HDRキャンバスの設定をベースにする
	uavDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // ★書き込み可能(UAV)フラグを立てる

	hr = device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &uavDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS, // 初期状態は書き込み待ち
		nullptr,                               // UAVにはクリアカラーは不要
		IID_PPV_ARGS(&uavTextureResource_));

	assert(SUCCEEDED(hr));

	// ==========================================
	// 6. ビューの生成
	// ==========================================
	// RTVの生成（変更なし）
	rtvHandle_ = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
	device->CreateRenderTargetView(hdrTextureResource_.Get(), nullptr, rtvHandle_);

	// SRVの生成（変更なし）
	srvHandle_ = srvHeap_->GetCPUDescriptorHandleForHeapStart();
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = resourceDesc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	device->CreateShaderResourceView(hdrTextureResource_.Get(), &srvDesc, srvHandle_);

	// UAVの生成（★新規追加）
	D3D12_CPU_DESCRIPTOR_HANDLE uavHandle = srvHandle_;
	// ヒープ内で次の空き部屋（2番目）にポインタを進める
	uavHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavViewDesc{};
	uavViewDesc.Format = resourceDesc.Format;
	uavViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	device->CreateUnorderedAccessView(uavTextureResource_.Get(), nullptr, &uavViewDesc, uavHandle);

	// ★追加：計算結果のテクスチャを、読み取り用(SRV)としても使えるようにする
	D3D12_CPU_DESCRIPTOR_HANDLE resultSrvHandle = uavHandle;
	resultSrvHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	device->CreateShaderResourceView(uavTextureResource_.Get(), &srvDesc, resultSrvHandle);
	// ==========================================
	// 7. ルートシグネチャの作成 (C++とシェーダーの橋渡し)
	// ==========================================
	D3D12_DESCRIPTOR_RANGE descriptorRange[2] = {};

	// SRV用 (読み込み: t0)
	descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange[0].NumDescriptors = 1;
	descriptorRange[0].BaseShaderRegister = 0;
	descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// UAV用 (書き込み: u0)
	descriptorRange[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	descriptorRange[1].NumDescriptors = 1;
	descriptorRange[1].BaseShaderRegister = 0;
	descriptorRange[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// ★ 変更：パラメータを3つにする
	D3D12_ROOT_PARAMETER rootParameters[3] = {};

	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[0].DescriptorTable.pDescriptorRanges = &descriptorRange[0];
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
	rootParameters[1].DescriptorTable.pDescriptorRanges = &descriptorRange[1];
	rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// ✨ 超便利機能「ルート定数」：バッファを作らずに直接intを3つ送る設定
	rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParameters[2].Constants.ShaderRegister = 0; // b0に送る
	rootParameters[2].Constants.Num32BitValues = 3; // 3つのデータを送る
	rootParameters[2].Constants.RegisterSpace = 0;
	rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
	rootSignatureDesc.NumParameters = 3; // ★ここも3にする
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
	hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signatureBlob, &errorBlob);
	assert(SUCCEEDED(hr));
	hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
	assert(SUCCEEDED(hr));

	// ==========================================
	// 8. パイプラインステートの作成 (CSの読み込みと設定)
	// ==========================================
	Microsoft::WRL::ComPtr<IDxcBlob> csBlob = dxCommon_->CompilerShader(
		L"Resources/Shaders/Bloom.hlsl", L"cs_6_0", // コンピュートシェーダーなので cs_6_0
		dxCommon_->GetDxcUtils(), dxCommon_->GetDxcCompiler(), dxCommon_->GetIncludeHandler());
	assert(csBlob != nullptr);

	D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineStateDesc{};
	pipelineStateDesc.pRootSignature = rootSignature_.Get();
	pipelineStateDesc.CS.pShaderBytecode = csBlob->GetBufferPointer();
	pipelineStateDesc.CS.BytecodeLength = csBlob->GetBufferSize();

	hr = device->CreateComputePipelineState(&pipelineStateDesc, IID_PPV_ARGS(&pipelineState_));
	assert(SUCCEEDED(hr));

	// ==========================================
	// 9. ポストプロセス用ルートシグネチャの作成
	// ==========================================
	D3D12_DESCRIPTOR_RANGE ppRange{};
	ppRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	ppRange.NumDescriptors = 1;
	ppRange.BaseShaderRegister = 0; // シェーダーのt0に送る
	ppRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// パラメータ0: 画像データ (t0)
	D3D12_ROOT_PARAMETER ppParamSRV{};
	ppParamSRV.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	ppParamSRV.DescriptorTable.NumDescriptorRanges = 1;
	ppParamSRV.DescriptorTable.pDescriptorRanges = &ppRange;
	ppParamSRV.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// パラメータ1: 定数データ (b0) - 時間やエフェクト強度を送る
	D3D12_ROOT_PARAMETER ppParamConstants{};
	ppParamConstants.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	ppParamConstants.Constants.ShaderRegister = 0; // b0
	ppParamConstants.Constants.Num32BitValues = 4; // float 3つ分 (時間, 色収差, ノイズ)
	ppParamConstants.Constants.RegisterSpace = 0;
	ppParamConstants.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// 🌟 修正の要：必ずこの配列(サイズ2)を作り、それを渡す！
	D3D12_ROOT_PARAMETER ppRootParams[2] = { ppParamSRV, ppParamConstants };

	// 画像を滑らかに補間するサンプラー（s0）
	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC ppRootDesc{};
	ppRootDesc.NumParameters = 2;          // パラメータは2つ
	ppRootDesc.pParameters = ppRootParams; // 🌟 古い変数が残らないよう、必ず作った配列を指定する！
	ppRootDesc.NumStaticSamplers = 1;
	ppRootDesc.pStaticSamplers = &samplerDesc;
	ppRootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	Microsoft::WRL::ComPtr<ID3DBlob> ppSigBlob, ppErrBlob;
	hr = D3D12SerializeRootSignature(&ppRootDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &ppSigBlob, &ppErrBlob);
	assert(SUCCEEDED(hr));
	hr = device->CreateRootSignature(0, ppSigBlob->GetBufferPointer(), ppSigBlob->GetBufferSize(), IID_PPV_ARGS(&postProcessRootSignature_));
	assert(SUCCEEDED(hr));

	// ==========================================
	// 10. ポストプロセス用パイプラインステートの作成
	// ==========================================
	Microsoft::WRL::ComPtr<IDxcBlob> vsBlob =
		dxCommon_->CompilerShader(L"Resources/Shaders/PostProcess.VS.hlsl", L"vs_6_0", dxCommon_->GetDxcUtils(), dxCommon_->GetDxcCompiler(), dxCommon_->GetIncludeHandler());
	assert(vsBlob != nullptr);

	Microsoft::WRL::ComPtr<IDxcBlob> psBlob =
		dxCommon_->CompilerShader(L"Resources/Shaders/PostProcess.PS.hlsl", L"ps_6_0", dxCommon_->GetDxcUtils(), dxCommon_->GetDxcCompiler(), dxCommon_->GetIncludeHandler());
	assert(psBlob != nullptr);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC ppPsoDesc{};
	ppPsoDesc.pRootSignature = postProcessRootSignature_.Get();
	ppPsoDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();
	ppPsoDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
	ppPsoDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();
	ppPsoDesc.PS.BytecodeLength = psBlob->GetBufferSize();
	ppPsoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	ppPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // カリングなし
	ppPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	ppPsoDesc.RasterizerState.DepthClipEnable = true;
	ppPsoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	ppPsoDesc.BlendState.RenderTarget[0].BlendEnable = TRUE;
	ppPsoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;  // 光(Bloom結果)の強さはそのまま
	ppPsoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE; // 下地(黄色いビール)にそのまま足す
	ppPsoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	ppPsoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	ppPsoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	ppPsoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

	ppPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	ppPsoDesc.NumRenderTargets = 1;
	// ★エンジンに合わせてフォーマットを設定
	ppPsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	ppPsoDesc.SampleDesc.Count = 1;

	ppPsoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	// ただし、画面全体にペタッと貼るだけなので深度のテスト自体はOFFにする
	ppPsoDesc.DepthStencilState.DepthEnable = FALSE;

	hr = device->CreateGraphicsPipelineState(&ppPsoDesc, IID_PPV_ARGS(&postProcessPipelineState_));
	assert(SUCCEEDED(hr));

}

ID3D12GraphicsCommandList* Bloom::GetCommandList() { return dxCommon_->GetCommandList(); }

void Bloom::PreDraw() {
	ID3D12GraphicsCommandList* cmdList = GetCommandList();

	// 1. リソースバリア：状態を「読み取り用(SRV)」から「書き込み用(RTV)」へ変更
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = hdrTextureResource_.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	cmdList->ResourceBarrier(1, &barrier);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dxCommon_->GetDsvDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();

	// 2. 描画先をこのHDRキャンバスに変更
	cmdList->OMSetRenderTargets(1, &rtvHandle_, FALSE, &dsvHandle);

	// 3. キャンバスを真っ黒にクリア（背景色）
	float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	cmdList->ClearRenderTargetView(rtvHandle_, clearColor, 0, nullptr);
}

void Bloom::PostDraw() {
	ID3D12GraphicsCommandList* cmdList = GetCommandList();

	// リソースバリア：状態を「書き込み用(RTV)」から「読み取り用(SRV)」へ戻す
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = hdrTextureResource_.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	cmdList->ResourceBarrier(1, &barrier);
}

void Bloom::Execute() {
	ID3D12GraphicsCommandList* cmdList = GetCommandList();
	ID3D12Device* device = GetDevice();

	// 1. パイプラインとルートシグネチャのセット
	cmdList->SetPipelineState(pipelineState_.Get());
	cmdList->SetComputeRootSignature(rootSignature_.Get());

	// 2. デスクリプタヒープのセット（SRVとUAVが入っている箱）
	ID3D12DescriptorHeap* ppHeaps[] = { srvHeap_.Get() };
	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// 3. GPUに渡すアドレス（ハンドル）の計算
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = srvHeap_->GetGPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE uavGpuHandle = srvGpuHandle;
	// UAVは2番目の部屋にあるので、1つ分アドレスを進める
	uavGpuHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 4. ルートパラメータにアドレスをセット（t0 = 読み取り, u0 = 書き込み）
	cmdList->SetComputeRootDescriptorTable(0, srvGpuHandle);
	cmdList->SetComputeRootDescriptorTable(1, uavGpuHandle);

	int constants[3] = { enableLuminance_ ? 1 : 0, enableBlur_ ? 1 : 0, enableAdditive_ ? 1 : 0 };
	// 配列ごとGPU（パラメータ2番 = b0レジスタ）に直接発射！
	cmdList->SetComputeRoot32BitConstants(2, 3, constants, 0);

	// 配列ごとGPU（パラメータ2番 = b0レジスタ）に直接発射！
	cmdList->SetComputeRoot32BitConstants(2, 3, constants, 0);
	// 5. コンピュートシェーダーの実行 (Dispatch)
	// 画面サイズ(1280x720)を、シェーダーで指定したスレッド数(8x8)で割って実行回数を決める
	UINT dispatchX = (1280 + 7) / 8;
	UINT dispatchY = (720 + 7) / 8;
	cmdList->Dispatch(dispatchX, dispatchY, 1);
}

void Bloom::DrawResult() {
	ID3D12GraphicsCommandList* cmdList = GetCommandList();
	ID3D12Device* device = GetDevice();

	// ==========================================================
	// ★追加：描画先を「本来の画面（モニター）」に切り替える
	// ==========================================================
	// これにより、これ以降の描画命令がモニター（バックバッファ）に届くようになります
	dxCommon_->SetBackBufferRenderTarget();

	// 1. リソースバリア：結果画像を UAV（書き込み） から SRV（読み取り） に切り替える
	D3D12_RESOURCE_BARRIER barrierToSRV{};
	barrierToSRV.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierToSRV.Transition.pResource = uavTextureResource_.Get();
	barrierToSRV.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	barrierToSRV.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrierToSRV.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	cmdList->ResourceBarrier(1, &barrierToSRV);

	// 2. 画面描画用のパイプラインを設定
	cmdList->SetGraphicsRootSignature(postProcessRootSignature_.Get());
	cmdList->SetPipelineState(postProcessPipelineState_.Get());
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 3. 画像が入っているヒープをセットし、3番目の部屋（結果画像）のアドレスを渡す
	ID3D12DescriptorHeap* ppHeaps[] = { srvHeap_.Get() };
	cmdList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	D3D12_GPU_DESCRIPTOR_HANDLE resultGpuHandle = srvHeap_->GetGPUDescriptorHandleForHeapStart();
	resultGpuHandle.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 2;

	cmdList->SetGraphicsRootDescriptorTable(0, resultGpuHandle);

	time_ += 1.0f / 60.0f;
	if (time_ > 1000.0f)
		time_ = 0.0f; // オーバーフロー防止

	// 時間とエフェクトの強さをHLSL (b0) に発射！
	float ppData[4] = { time_, chromaticAberration_, noiseIntensity_, useACES_ ? 1.0f : 0.0f };

	cmdList->SetGraphicsRoot32BitConstants(1, 4, ppData, 0);

	//  画面全体を覆う巨大な三角形を描画
	cmdList->DrawInstanced(3, 1, 0, 0);

	// 5. リソースバリア：次のフレームのために UAV（書き込み） に戻す
	D3D12_RESOURCE_BARRIER barrierToUAV = barrierToSRV;
	barrierToUAV.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	barrierToUAV.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	cmdList->ResourceBarrier(1, &barrierToUAV);
}

void Bloom::DrawImGui() {
	if (ImGui::TreeNodeEx("Bloom Debug Settings")) {
		ImGui::Checkbox("1. Luminance Extraction ", &enableLuminance_);
		ImGui::Checkbox("2. Gaussian Blur ", &enableBlur_);
		ImGui::Checkbox("3. Additive Blending", &enableAdditive_);
		ImGui::Separator();
		ImGui::Text("Camera Lens Effects");

		ImGui::SliderFloat("Chromatic Aberration", &chromaticAberration_, 0.0f, 0.05f);
		ImGui::SliderFloat("Film Grain", &noiseIntensity_, 0.0f, 0.5f);

		ImGui::Checkbox("Use ACES Tonemapping", &useACES_);
		ImGui::TreePop();
	}
}