#pragma once
#include "Base/DirectXCommon.h"
#include <wrl.h>
#include "Logger.h"
#include <string>
#include "Matrix4x4.h"
#include "Vector4.h"
#include "Vector3.h"



#include "Model.h"


class Object3d {

public:
	struct Transform {
		Vector3  scale;
		Vector3 rotate;
		Vector3 translate;
	};

	struct Material {
		Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
	};

	struct DirectionalLight {
		Vector4 color;
		Vector3 direction;
		float intensity;
	};

	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
	};

	/*-----------------------
	メッシュごとのリソースを管理する構造体
	----------------------------------------------*/
	struct MeshResource {
		Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
		Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
		Material* materialData = nullptr;
		uint32_t vertexCount = 0;
		Microsoft::WRL::ComPtr<ID3D12Resource> textureResource;
		D3D12_GPU_DESCRIPTOR_HANDLE textureHandleGPU{}; // このパーツが使うテクスチャのハンドル
	};

	void Initialize(DirectXCommon* dxCommon);
	void Update();
	void Draw();


	void SetTextureHandle(uint32_t handle) { textureHandle_ = handle; }

	
	Transform& GetTransform() { return transform_; }

	void SetCameraMatrix(const Matrix4x4& view, const Matrix4x4& projection) {
		viewMatrix_ = view;
		projectionMatrix_ = projection;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSrvHandleGPU() const { return textureSrvHandleGPU_; }

	// ゲッター
	ID3D12RootSignature* GetRootSignature() { return rootSignature_.Get(); }
	ID3D12PipelineState* GetGraphicsPipelineState()const { return graphicsPipelineState_.Get(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSrvHandleGPU2() const { return textureSrvHandleGPU2_; }

	Material* GetMaterialData() { return meshResources_[0].materialData; }

private:
	DirectXCommon* dxCommon_ = nullptr;

	// rootSignatureとGraphicPipelineState
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;

	std::vector<MeshResource> meshResources_;

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);

	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;

	// 自分の位置　回転スケールを持つ変数
	Transform transform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	Transform cameraTransform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -5.0f} };

	TransformationMatrix* wvpData_ = nullptr;

	// テクスチャ追加用のリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource_;

	// テクスチャのGPU上のアドレス
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU_;

	Vector4 materialColor_ = { 1.0f, 1.0f, 1.0f, 1.0f };

	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2_;
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource2_;

	bool useMonsterBall_ = true;

	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
	DirectionalLight* directionalLightData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
	Transform uvTransform_{
		{ 1.0f, 1.0f, 1.0f }, // scale
		{ 0.0f, 0.0f, 0.0f }, // rotate
		{ 0.0f, 0.0f, 0.0f }  // translate
	};

	Matrix4x4 viewMatrix_ = MakeIdentity4x4();
	Matrix4x4 projectionMatrix_ = MakeIdentity4x4();


	uint32_t textureHandle_ = 0;
};