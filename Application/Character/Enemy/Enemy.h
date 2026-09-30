#pragma once
#include "3D/Object3d.h"

class Enemy {
public:
	// デストラクタ
	~Enemy() { delete model3D_; }

	//　初期化
	void Initialize(const Vector3& position);

	// 更新処理
	void Update();

	// 描画処理
	void Draw(const WorldTransform& worldTransform, const ViewProjection& viewProjection, uint32_t textureHandle);

	// 座標のゲッター
	Vector3 GetPosition() const { return model3D_->GetTransform().translate; }
	bool IsDead() const { return isDead_; }
	void OnCollision();

private:
	// 敵の3Dモデル
	Object3d* model3D_ = nullptr;

	bool isDead_ = false;
	float speed_ = 0.05f;

	Vector3 gamePos_ = { 0.0f, 0.0f, 0.0f };

	void ApplyGlobalVariables();
};