#pragma once
#include "Object3d.h"
#include "Vector3.h"
#include "ViewProjection.h"
#include "WorldTransform.h"


class EnemyBullet {
public:
	// 初期化処理
	void Initialize(Object3d* model, const Vector3 position, const Vector3& velocity);

	// 更新処理
	void Update();

	void Draw(const ViewProjection& camera);

private:
	WorldTransform worldTransform_;
	Object3d* model_ = nullptr;
	uint32_t textureHandle_ = 0u;

	// 速度
	Vector3 velocity_;
};