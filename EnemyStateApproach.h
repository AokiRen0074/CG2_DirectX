#pragma once
#include "BaseEnemyState.h"
#include "Vector3.h"

class EnemyStateApproach : public BaseEnemyState {
public:
	void Update() override;

private:

	// 接近速度
	static inline Vector3 approachVelocity_ = { 0.0f,0.0f,-0.1f };
};
