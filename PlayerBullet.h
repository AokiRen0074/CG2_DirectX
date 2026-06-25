#pragma once

#include "Object3d.h"
#include "Vector3.h"
#include "ViewProjection.h"
#include "WorldTransform.h"
#include "TextureManager.h"

// 親クラス
#include "BaseCharacter.h"

class PlayerBullet : public BaseCharacter {

public:

	/*----------------------
	弾
	---------------------*/

	// 更新処理
	void Initialize(Object3d* model, const Vector3& position, const Vector3& velocity);

	// 更新処理
	void Update();

	// デスフラグのゲッター
	bool IsDead() const { return isDead_; }

private:
	Vector3 velocity_;

	/*----------------------
	弾
	-----------------------*/
	// 寿命
	static const int32_t kLifeTime = 60 * 5;

	// デスタイマー
	int32_t deathTimer_ = kLifeTime;

	// デスフラグ
	bool isDead_ = false;

};