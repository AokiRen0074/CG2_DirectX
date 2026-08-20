#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "Math/Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include "Matrix4x4.h"
#include "DirectXCommon.h"
#include "Transform.h"

// 頂点データの構造体
struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

struct Material {
	Vector4 color;
	int32_t enableLighting;
	float padding[3];
	Matrix4x4 uvTransform; //UV変換
};

struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 World;
};



class Sprite {
public:

	static void StaticInitialize(DirectXCommon* dxCommon);

	static Sprite* Create(uint32_t textureHandle, Vector2 position);

	void Initialize(DirectXCommon* dxCommon, uint32_t textureHandle);
	void Update();
	void Draw();

	void SetPosition(const Vector2& position);

	void SetScale(const Vector2& scale);
	void SetRotation(float rotation);

	// 位置や大きさを変えるためのゲッター・セッター
	struct Transform& GetTransform() { return transform_; }

private:

	DirectXCommon* dxCommon_ = nullptr;

	uint32_t textureHandle_ = 0;

	// 頂点データ関連
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

	// 行列データ関連
	Microsoft::WRL::ComPtr<ID3D12Resource> transformationMatrixResource_;
	TransformationMatrix* transformationMatrixData_ = nullptr;
	struct Transform transform_ = { {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };



	// リソース作成用の便利関数
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
	Material* materialData_ = nullptr;

	// SpriteをIndex描画に変更する
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSprite_;
	D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite_{};

	struct Transform uvTransformSprite_ {
		{ 1.0f, 1.0f, 1.0f }, //sale
		{ 0.0f,0.0f,0.0f },// rotate
		{ 0.0f,0.0f,0.0f }//taransrate
	};

	static DirectXCommon* sDxCommon_;

};