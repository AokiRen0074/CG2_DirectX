#pragma once
#include "Object3d.h"
#include "Vector3.h"
#include "ViewProjection.h"
#include "WorldTransform.h"

class Player;

class EnemyBullet {
public:
	// 初期化処理
	void Initialize(Object3d* model, const Vector3 position, const Vector3& velocity);

	// 更新処理
	void Update();

	void Draw(const ViewProjection& camera);

	// ゲッター
	bool IsDead() const { return isDead_; }

	// 自キャラのポインタを受け取る関数
	void SetPlayer(Player* player) { player_ = player; }

private:
	WorldTransform worldTransform_;
	Object3d* model_ = nullptr;
	uint32_t textureHandle_ = 0u;

	// 速度
	Vector3 velocity_;

	// 弾の寿命
	static const int32_t kLifeTime = 60 * 5;

	// デスタイマー
	int32_t deathTimer_ = kLifeTime;

	// デスフラグ
	bool isDead_ = false;


	Player* player_;

};