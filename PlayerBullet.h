#pragma once

#include "NeonModel.h"
#include "Vector3.h"
#include "ViewProjection.h"
#include "WorldTransform.h"
#include "TextureManager.h"
#include <list>
#include "NeonModel.h"

// 親クラス
#include "BaseCharacter.h"
#include "Collider.h"

class Enemy;

class PlayerBullet : public BaseCharacter, public Collider{

public:

	/*----------------------
	弾
	---------------------*/

	// 更新処理
	void Initialize(NeonModel* model, const Vector3& position, const Vector3& velocity, const Vector3& rotation);


	// 更新処理
	void Update(const std::list<Enemy*>& enemies);

	void Draw(const ViewProjection& viewProjection) override;

	// デスフラグのゲッター
	bool IsDead() const { return isDead_; }



	void OnCollision() override;
	Vector3 GetWorldPosition() override;

private:
	

	/*----------------------
	弾
	-----------------------*/
	Vector3 velocity_;
	NeonModel* neonModel_ = nullptr;

	Enemy* target_ = nullptr;

	// 軌道の履歴を保存するリスト
	std::list<Vector3> trailHistory_;
	static const int32_t kMaxTrail = 10; // 軌道の長さ

	WorldTransform trailTransforms_[kMaxTrail];

	static const int32_t kLifeTime = 60 * 5;
	int32_t deathTimer_ = kLifeTime;
	bool isDead_ = false;




};