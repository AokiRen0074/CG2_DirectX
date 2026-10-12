#pragma once
#include "Model.h"
#include <memory>

// 読み込み済みモデルの頂点バッファ
struct ModelMeshResource {
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	uint32_t vertexCount = 0;
};

struct ModelVertexResources {
	std::shared_ptr<const ModelData> modelData;
	std::vector<ModelMeshResource> meshes;
};

// モデルデータと頂点バッファを一括管理する
// 同じファイルを使うオブジェクトは、読み込み済みのデータを共有する
class ModelManager {
public:
	static ModelManager* GetInstance();

	static std::shared_ptr<const ModelData> Load(
		const std::string& directoryPath, const std::string& filename);

	static std::shared_ptr<const ModelVertexResources> GetVertexResources(
		ID3D12Device* device, const std::shared_ptr<const ModelData>& model);

	static void Finalize();

private:
	ModelManager() = default;
	ModelManager(const ModelManager&) = delete;
	ModelManager& operator=(const ModelManager&) = delete;

	std::map<std::string, std::shared_ptr<const ModelData>> loadedModels_;
	std::map<ID3D12Device*,
		std::map<const ModelData*, std::shared_ptr<const ModelVertexResources>>> vertexResources_;
};