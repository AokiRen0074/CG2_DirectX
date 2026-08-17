#pragma once
#include <list>
#include "NeonParticle.h"

class ParticleManager {
public:
	void Initialize(NeonModel* model, uint32_t textureHandle);
	void Update();
	void Draw(const ViewProjection& viewProjection);

	// 指定した座標に、指定した数と色のパーティクルをばらまく
	void Emit(const Vector3& position, int count, const Vector3& color);

private:
	std::list<NeonParticle*> particles_;
	NeonModel* particleModel_ = nullptr;
	uint32_t textureHandle_ = 0;
};