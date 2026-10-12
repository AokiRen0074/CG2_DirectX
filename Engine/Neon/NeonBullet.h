#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "Vector4.h"
#include "Vector3.h"
#include "Vector2.h"
#include "Matrix4x4.h"
#include "DirectXCommon.h"


class NeonBullet {
public:
	void Initialize(DirectXCommon* dxCommon);
	void Update(float shakeX, float shakeY);
	void Draw();

	void SetTransform( Vector3 pos,  Vector3 scale) {
		position_ = pos;
		scale_ = scale;
	}

	// 画面外に出たかどうかのフラグを取得する
	bool IsDead() const { return isDead_; }

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vbView_{};
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;
	D3D12_INDEX_BUFFER_VIEW ibView_{};

	Microsoft::WRL::ComPtr<ID3D12Resource> cbTransform_;
	Microsoft::WRL::ComPtr<ID3D12Resource> cbColor_;

	struct Vertex {
		 Vector3 pos;
		 Vector2 uv;
	};

	struct TransformData {
		 Matrix4x4 WVP;
	};
	struct ColorData {
		 Vector4 color;
		float intensity;
		float radius;
		float softness;
		float length;
	};

	TransformData* mappedTransform_ = nullptr;
	ColorData* mappedColor_ = nullptr;

	 Vector3 position_ = {0.0f, 0.0f, 0.0f};
	 Vector3 scale_ = {1.0f, 1.0f, 1.0f};
	bool isDead_ = false; // 消滅フラグ

	// 弾の色（明るい黄色や水色などがおすすめ）
	float bulletColor_[3] = {1.0f, 0.8f, 0.2f};
	float bulletIntensity_ = 10.0f;
	float bulletRadius_ = 0.05f;

	DirectXCommon* dxCommon_ = nullptr;
};