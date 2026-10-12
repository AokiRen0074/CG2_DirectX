#include "ModelManager.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <limits>
#include <stdexcept>

ModelManager* ModelManager::GetInstance() {
	static ModelManager instance;
	return &instance;
}

std::shared_ptr<const ModelData> ModelManager::Load(
	const std::string& directoryPath, const std::string& filename) {
	// 相対パスの表記違いでも、同じファイルとして管理する
	const auto filePath = std::filesystem::weakly_canonical(
		std::filesystem::path(directoryPath) / filename);
	std::string key = filePath.generic_string();

#ifdef _WIN32
	std::transform(key.begin(), key.end(), key.begin(), [](unsigned char value) {
		return static_cast<char>(std::tolower(value));
	});
#endif

	auto& loadedModels = GetInstance()->loadedModels_;
	auto found = loadedModels.find(key);

	if (found != loadedModels.end()) {
		return found->second;
	}

	// ファイルからの読み込みは、最初の1回だけ行う
	auto model = std::make_shared<const ModelData>(
		LoadObjectFile(filePath.parent_path().string(), filePath.filename().string()));
	loadedModels.emplace(key, model);

	return model;
}

std::shared_ptr<const ModelVertexResources> ModelManager::GetVertexResources(
	ID3D12Device* device, const std::shared_ptr<const ModelData>& model) {
	if (!device || !model) {
		throw std::invalid_argument("Model vertex resources require a device and model");
	}

	auto& cachedResources = GetInstance()->vertexResources_[device];
	auto found = cachedResources.find(model.get());

	if (found != cachedResources.end()) {
		return found->second;
	}

	auto resources = std::make_shared<ModelVertexResources>();
	resources->modelData = model;
	resources->meshes.reserve(model->meshes.size());

	for (const auto& mesh : model->meshes) {
		ModelMeshResource meshResource;

		// 頂点のないパーツにはバッファを作らない
		if (!mesh.vertices.empty()) {
			const size_t bufferSize = sizeof(VertexData) * mesh.vertices.size();

			if (bufferSize > (std::numeric_limits<UINT>::max)()) {
				throw std::length_error("Model vertex buffer is too large");
			}

			D3D12_HEAP_PROPERTIES heapProperties{};
			heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

			D3D12_RESOURCE_DESC resourceDesc{};
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourceDesc.Width = bufferSize;
			resourceDesc.Height = 1;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

			HRESULT hr = device->CreateCommittedResource(
				&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
				IID_PPV_ARGS(&meshResource.vertexResource));

			if (FAILED(hr)) {
				throw std::runtime_error("Failed to create model vertex buffer");
			}

			void* mappedData = nullptr;
			hr = meshResource.vertexResource->Map(0, nullptr, &mappedData);

			if (FAILED(hr)) {
				throw std::runtime_error("Failed to map model vertex buffer");
			}

			std::memcpy(mappedData, mesh.vertices.data(), bufferSize);
			meshResource.vertexResource->Unmap(0, nullptr);

			meshResource.vertexCount = static_cast<uint32_t>(mesh.vertices.size());
			meshResource.vertexBufferView.BufferLocation =
				meshResource.vertexResource->GetGPUVirtualAddress();
			meshResource.vertexBufferView.SizeInBytes = static_cast<UINT>(bufferSize);
			meshResource.vertexBufferView.StrideInBytes = sizeof(VertexData);
		}

		resources->meshes.push_back(std::move(meshResource));
	}

	cachedResources.emplace(model.get(), resources);

	return resources;
}

void ModelManager::Finalize() {
	// GPUリソースを先に解放し、その後にファイルから読んだデータを解放する
	GetInstance()->vertexResources_.clear();
	GetInstance()->loadedModels_.clear();
}