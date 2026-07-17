#pragma once
#include "Base/DirectXCommon.h"
#include <wrl.h>
#include "Logger.h"
#include "Matrix4x4.h"
#include "Vector4.h"
#include "Vector3.h"
#include "WorldTransform.h"
#include "ViewProjection.h"
#include "Model.h"

class Sphere {
public:
	struct Transform {
		Vector3 scale;
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
		int32_t lightingType;
		float padding2[3];
	};

	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
	};

	// 静的初期化
	static void StaticInitialize(DirectXCommon* dxCommon);

	// 初期化
	void Initialize();
	// 更新
	void Update();
	// 描画
	void Draw(const WorldTransform& worldTransform, const ViewProjection& viewProjection, uint32_t textureHandle);

	void SetCameraMatrix(const Matrix4x4& view, const Matrix4x4& projection) {
		viewMatrix_ = view;
		projectionMatrix_ = projection;
	}

	Transform& GetTransform() { return transform_; }

private:
	static DirectXCommon* sDxCommon_;
	DirectXCommon* dxCommon_ = nullptr;

	// パイプライン関連
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;

	// 球体の頂点とインデックス
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_{};
	uint32_t indexCount_ = 0;

	// 各種リソース
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	Material* materialData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource_;
	DirectionalLight* directionalLightData_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource_;
	TransformationMatrix* wvpData_ = nullptr;

	// トランスフォーム
	Transform transform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	Transform cameraTransform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -5.0f} };

	Matrix4x4 viewMatrix_ = MakeIdentity4x4();
	Matrix4x4 projectionMatrix_ = MakeIdentity4x4();

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);
};