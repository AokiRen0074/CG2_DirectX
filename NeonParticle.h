#pragma once
#include "WorldTransform.h"
#include "NeonModel.h"
#include "ViewProjection.h"

class NeonParticle {
public:
	// 初期化
	void Initialize(NeonModel* model, const Vector3& position, const Vector3& direction, float speed, float thickness, const Vector3& color);
	void Update();
	void Draw(const ViewProjection& viewProjection, uint32_t textureHandle);

	bool IsDead() const { return life_ <= 0; }

private:
	WorldTransform worldTransform_;
	NeonModel* model_ = nullptr;

	Vector3 velocity_;
	float speed_ = 0.0f;
	float thickness_ = 0.0f;
	Vector3 color_;

	int life_ = 0;
	int maxLife_ = 0;
};