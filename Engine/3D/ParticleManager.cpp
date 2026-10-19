#include "ParticleManager.h"
#include "2D/TextureManager.h"
#include "WindowApp.h"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace {

void CheckParticleResult(HRESULT result) {
	if (FAILED(result)) {
		throw std::runtime_error("Failed to create particle rendering resources");
	}
}

Vector4 ToClipPosition(const Vector3& position, const Matrix4x4& matrix) {
	Vector4 result{};
	result.x = position.x * matrix.m[0][0] + position.y * matrix.m[1][0] +
		position.z * matrix.m[2][0] + matrix.m[3][0];
	result.y = position.x * matrix.m[0][1] + position.y * matrix.m[1][1] +
		position.z * matrix.m[2][1] + matrix.m[3][1];
	result.z = position.x * matrix.m[0][2] + position.y * matrix.m[1][2] +
		position.z * matrix.m[2][2] + matrix.m[3][2];
	result.w = position.x * matrix.m[0][3] + position.y * matrix.m[1][3] +
		position.z * matrix.m[2][3] + matrix.m[3][3];
	return result;
}

}

void ParticleManager::Initialize(DirectXCommon* dxCommon) {
	if (!dxCommon) {
		throw std::invalid_argument("Particles require DirectXCommon");
	}

	if (dxCommon_) {
		throw std::logic_error("ParticleManager is already initialized");
	}

	// テクスチャだけをルートパラメーターとして受け取る
	D3D12_DESCRIPTOR_RANGE range{};
	range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	range.NumDescriptors = 1;
	range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER parameter{};
	parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	parameter.DescriptorTable.NumDescriptorRanges = 1;
	parameter.DescriptorTable.pDescriptorRanges = &range;

	D3D12_STATIC_SAMPLER_DESC sampler{};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootDesc{};
	rootDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootDesc.NumParameters = 1;
	rootDesc.pParameters = &parameter;
	rootDesc.NumStaticSamplers = 1;
	rootDesc.pStaticSamplers = &sampler;

	Microsoft::WRL::ComPtr<ID3DBlob> signature, errors;
	CheckParticleResult(D3D12SerializeRootSignature(
		&rootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &errors));
	CheckParticleResult(dxCommon->GetDevice()->CreateRootSignature(
		0, signature->GetBufferPointer(), signature->GetBufferSize(),
		IID_PPV_ARGS(&rootSignature_)));

	auto vertexShader = dxCommon->CompilerShader(
		L"Resources/Shaders/Particle.VS.hlsl", L"vs_6_0",
		dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler());
	auto pixelShader = dxCommon->CompilerShader(
		L"Resources/Shaders/Particle.PS.hlsl", L"ps_6_0",
		dxCommon->GetDxcUtils(), dxCommon->GetDxcCompiler(), dxCommon->GetIncludeHandler());

	D3D12_INPUT_ELEMENT_DESC elements[3]{};
	elements[0].SemanticName = "POSITION";
	elements[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	elements[1].SemanticName = "TEXCOORD";
	elements[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	elements[2].SemanticName = "COLOR";
	elements[2].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

	for (auto& element : elements) {
		element.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	}

	// 3Dの奥行きは参照するが、半透明の粒で深度を書き換えない
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
	desc.pRootSignature = rootSignature_.Get();
	desc.InputLayout = { elements, _countof(elements) };
	desc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
	desc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };
	desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	desc.RasterizerState.DepthClipEnable = TRUE;
	desc.DepthStencilState.DepthEnable = TRUE;
	desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
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
	CheckParticleResult(dxCommon->GetDevice()->CreateGraphicsPipelineState(
		&desc, IID_PPV_ARGS(&pipelineState_)));
	dxCommon_ = dxCommon;
}

void ParticleManager::CreateGroup(const std::string& name, const std::string& texturePath) {
	if (!dxCommon_ || name.empty() || groups_.contains(name)) {
		throw std::invalid_argument("Particle group requires initialization and a unique name");
	}

	Group group;
	group.textureHandle = TextureManager::Load(texturePath);
	group.particles.reserve(kMaxParticlesPerGroup);

	// 粒が増えるたびにリソースを作らず、グループごとに頂点バッファを確保する
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = sizeof(Vertex) * kMaxParticlesPerGroup * 6;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	CheckParticleResult(dxCommon_->GetDevice()->CreateCommittedResource(
		&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&group.vertexResource)));
	CheckParticleResult(group.vertexResource->Map(
		0, nullptr, reinterpret_cast<void**>(&group.vertices)));

	group.vertexBufferView.BufferLocation = group.vertexResource->GetGPUVirtualAddress();
	group.vertexBufferView.SizeInBytes = static_cast<UINT>(resourceDesc.Width);
	group.vertexBufferView.StrideInBytes = sizeof(Vertex);
	groups_.emplace(name, std::move(group));
}

size_t ParticleManager::Emit(
	const std::string& name, const ParticleParameters& parameters, size_t count) {
	Particle particle(parameters);
	auto& group = groups_.at(name);
	const size_t available = kMaxParticlesPerGroup - group.particles.size();
	const size_t emittedCount = (std::min)(count, available);

	for (size_t i = 0; i < emittedCount; ++i) {
		group.particles.push_back(particle);
	}

	return emittedCount;
}

void ParticleManager::Update(float deltaTime) {
	if (!std::isfinite(deltaTime) || deltaTime < 0.0f) {
		throw std::invalid_argument("Invalid particle update interval");
	}

	for (auto& [name, group] : groups_) {
		for (auto& particle : group.particles) {
			particle.Update(deltaTime);
		}

		std::erase_if(group.particles, [](const Particle& particle) {
			return !particle.IsAlive();
		});
	}
}

void ParticleManager::Draw(const ViewProjection& camera) {
	if (!dxCommon_) {
		throw std::logic_error("ParticleManager is not initialized");
	}

	const Matrix4x4 cameraWorld = Inverse(camera.matView);
	const Matrix4x4 viewProjection = Multiply(camera.matView, camera.matProjection);
	const Vector3 right = { cameraWorld.m[0][0], cameraWorld.m[0][1], cameraWorld.m[0][2] };
	const Vector3 up = { cameraWorld.m[1][0], cameraWorld.m[1][1], cameraWorld.m[1][2] };

	auto* commandList = dxCommon_->GetCommandList();

	// スプライトやImGuiと共存できるよう、必要な設定を毎回指定する
	commandList->SetGraphicsRootSignature(rootSignature_.Get());
	commandList->SetPipelineState(pipelineState_.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ID3D12DescriptorHeap* heaps[] = { dxCommon_->GetSrvDescriptorHeap() };
	commandList->SetDescriptorHeaps(1, heaps);

	const D3D12_VIEWPORT viewport = {
		0.0f, 0.0f, float(WindowApp::kClientWidth), float(WindowApp::kClientHeight), 0.0f, 1.0f
	};
	const D3D12_RECT scissorRect = { 0, 0, WindowApp::kClientWidth, WindowApp::kClientHeight };
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &scissorRect);

	constexpr std::array<Vector2, 4> corners = {{
		{-0.5f, 0.5f}, {-0.5f, -0.5f}, {0.5f, 0.5f}, {0.5f, -0.5f}
	}};
	constexpr std::array<Vector2, 4> uvs = {{
		{0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}
	}};
	constexpr std::array<size_t, 6> indices = { 0, 1, 2, 1, 3, 2 };

	for (auto& [name, group] : groups_) {
		if (group.particles.empty()) {
			continue;
		}

		size_t vertexCount = 0;

		for (const auto& particle : group.particles) {
			const auto& center = particle.GetPosition();
			const auto& size = particle.GetSize();

			for (const size_t index : indices) {
				const float x = corners[index].x * size.x;
				const float y = corners[index].y * size.y;
				const Vector3 position = {
					center.x + right.x * x + up.x * y,
					center.y + right.y * x + up.y * y,
					center.z + right.z * x + up.z * y
				};
				group.vertices[vertexCount++] = {
					ToClipPosition(position, viewProjection), uvs[index], particle.GetColor()
				};
			}
		}

		commandList->IASetVertexBuffers(0, 1, &group.vertexBufferView);
		commandList->SetGraphicsRootDescriptorTable(
			0, TextureManager::GetInstance()->GetSrvHandleGPU(group.textureHandle));
		commandList->DrawInstanced(static_cast<UINT>(vertexCount), 1, 0, 0);
	}
}

size_t ParticleManager::GetParticleCount(const std::string& name) const {
	return groups_.at(name).particles.size();
}

uint32_t ParticleManager::GetTextureHandle(const std::string& name) const {
	return groups_.at(name).textureHandle;
}