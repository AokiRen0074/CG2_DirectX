#pragma once
#include "Base/DirectXCommon.h"
#include <wrl.h>
#include "Logger.h"
#include <string>
#include "Matrix4x4.h"
#include "Vector4.h"
#include "Vector3.h"
#include "WorldTransform.h"
#include "Model.h"
#include <memory>
#include "SceneLighting.h"

enum class BlendMode {
	kNormal,
	kAdd,
	kSubtract,
	kMultiply,
	kScreen,
	kCount
};


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
		int32_t lightingType;
		float padding2[3];
		SceneLighting scene;
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
		Transform uvTransform = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
	};

	static void StaticInitialize(DirectXCommon* dxCommon);

	static Object3d* Create(const std::string& directoryPath, const std::string& filename);

	void Initialize(const std::string& directoryPath, const std::string& filename);
	void Update();

	// 初期化済みのオブジェクトが使うモデルを切り替える
	void SetModel(const std::string& directoryPath, const std::string& filename);
	void SetModel(const std::shared_ptr<const ModelData>& model);

	std::shared_ptr<const ModelData> GetModel() const {
		return model_;
	}

	// 各オブジェクト自身の座標を指定する
	void SetPosition(const Vector3& position);

	// ライトの方向は正規化する。ゼロ方向は既定の向きに戻す。
	void SetDirectionalLight(const Vector3& direction, const Vector3& color, float intensity);
	// 既定は無効。ステージの実体モデルにだけ周辺光と投影影を適用する。
	void SetSceneLighting(const SceneLighting& lighting);
	void SetScale(const Vector3& scale);
	void SetRotation(const Vector3& rotation);

	const Matrix4x4& GetWorldMatrix() const {
		return worldMatrix_;
	}

	// 自分のワールド行列と、指定されたカメラで描画する
	void Draw(const ViewProjection& viewProjection, uint32_t textureHandle);
	void Draw(const ViewProjection& viewProjection);

	// SetCameraMatrixで設定したカメラを使って描画する
	void Draw(uint32_t textureHandle);

	// テクスチャ有り
	void Draw(const WorldTransform& worldTransform, const ViewProjection& viewProjection, uint32_t textureHandle);

	// テクスチャなし
	void Draw(const WorldTransform& worldTransform, const ViewProjection& viewProjection);

	void SetCameraMatrix(const Matrix4x4& view, const Matrix4x4& projection) {
		viewMatrix_ = view;
		projectionMatrix_ = projection;
	}

	Transform& GetTransform() { return transform_; }

	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSrvHandleGPU() const { return textureSrvHandleGPU_; }

	// ゲッター
	ID3D12RootSignature* GetRootSignature() { return rootSignature_.Get(); }
	ID3D12PipelineState* GetGraphicsPipelineState()const { return graphicsPipelineState_.Get(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetTextureSrvHandleGPU2() const { return textureSrvHandleGPU2_; }

private:
	DirectXCommon* dxCommon_ = nullptr;

	// rootSignatureとGraphicPipelineState
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;

	// テクスチャがない時のパイプライン
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignatureNoTexture_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineStateNoTexture_;

	std::vector<MeshResource> meshResources_;

	// モデルデータは共有し、座標・マテリアル・ワールド行列は個別に持つ
	std::shared_ptr<const ModelData> model_;
	uint32_t defaultTextureHandle_ = 0;
	Matrix4x4 worldMatrix_ = MakeIdentity4x4();
	void UpdateMatrices(const Matrix4x4& parentMatrix);

	// 頂点データ
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

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

	// クラス全体で共有するdxCommon
	static DirectXCommon* sDxCommon_;

	// いま選択されているブレンドモード
	int blendMode_ = static_cast<int>(BlendMode::kNormal);

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineStates_[5];

};