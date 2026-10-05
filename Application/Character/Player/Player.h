#pragma once
#include "Engine/3D/Object3d.h"
#include "Engine/Input/input.h"

class Player {
public:
	// デストラクタ
	~Player() { delete model3D_; }

	// 更新処理
	void Initialize();

	// 更新
	void Update();


	// 描画
	void Draw(const WorldTransform& worldTransForm, const ViewProjection& viewProjection, uint32_t textureHandle);
	void DrawImGui();

	// 座標のゲッター
	Vector3 GetPosition()const { return model3D_->GetTransform().translate; }

	// 衝突時のリアクション
	void OnCollision();

private:
	Object3d* model3D_ = nullptr;
	float velocityY_ = 0.0f;
	bool isJumping_ = false;
	Vector3 gamePos_ = { 0.0f, 0.0f, 0.0f };

	void ApplyGlobalVariables();
};