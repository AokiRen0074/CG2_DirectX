#pragma once
#include "Particle.h"
#include "ViewProjection.h"
#include "Base/DirectXCommon.h"
#include <map>
#include <string>
#include <vector>

// テクスチャごとに粒を管理し、同じ画像の粒をまとめて描画する
class ParticleManager {
public:
	void Initialize(DirectXCommon* dxCommon);
	void CreateGroup(const std::string& name, const std::string& texturePath);
	size_t Emit(const std::string& name, const ParticleParameters& parameters, size_t count = 1);
	void Update(float deltaTime);
	void Draw(const ViewProjection& camera);

	size_t GetParticleCount(const std::string& name) const;
	uint32_t GetTextureHandle(const std::string& name) const;

	ParticleManager() = default;
	ParticleManager(const ParticleManager&) = delete;
	ParticleManager& operator=(const ParticleManager&) = delete;

private:
	struct Vertex {
		Vector4 position;
		Vector2 texcoord;
		Vector4 color;
	};

	struct Group {
		uint32_t textureHandle = 0;
		std::vector<Particle> particles;
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
		Vertex* vertices = nullptr;
	};

	static constexpr size_t kMaxParticlesPerGroup = 256;
	DirectXCommon* dxCommon_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
	std::map<std::string, Group> groups_;
};