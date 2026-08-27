#pragma once
#include "NeonModel.h"
#include "Vector3.h"
#include "ViewProjection.h"
#include "WorldTransform.h"
#include "Collider.h"

class Player;

class EnemyBullet: public Collider {
public:
	// 初期化処理
	void Initialize(NeonModel* model, const Vector3 position, const Vector3& velocity,uint32_t textureHandle);

	// 更新処理
	void Update();

	void Draw(const ViewProjection& camera);

	// ゲッター
	bool IsDead() const { return isDead_; }

	// 自キャラのポインタを受け取る関数
	void SetPlayer(Player* player) { player_ = player; }

	void SetStandby(bool standby) { isStandby_ = standby; }
	void SetVelocity(const Vector3& velocity) { velocity_ = velocity; }
	void SetPosition(const Vector3& position) { worldTransform_.translation_ = position; }

	void OnCollision() override;
	Vector3 GetWorldPosition() override;

private:
	WorldTransform worldTransform_;
	NeonModel* model_ = nullptr;
	uint32_t textureHandle_ = 0u;

	// 速度
	Vector3 velocity_;

	// 弾の寿命
	static const int32_t kLifeTime = 60 * 5;

	// デスタイマー
	int32_t deathTimer_ = kLifeTime;

	// デスフラグ
	bool isDead_ = false;

	Player* player_ = nullptr;

	float colorTimer_ = 0.0f;


	bool isStandby_ = false;

};