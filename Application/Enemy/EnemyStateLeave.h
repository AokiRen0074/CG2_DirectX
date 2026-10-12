#pragma once
#include "BaseEnemyState.h"
#include "Vector3.h"

class EnemyStateLeave : public BaseEnemyState {
public:
	void Update() override;

private:

		// 離脱速度
		static inline Vector3 leaveVelocity_ = { 0.0f, 0.0f, 0.1f };
};
