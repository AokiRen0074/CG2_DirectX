#include "EnemyStateHold.h"
#include "BaseEnemy.h"
#include "EnemyStateLeave.h"
#include <cmath>
#include "Application/Character/Player.h"

void EnemyStateHold::Update() {
	// 敵の現在のワールド座標
	Vector3 pos = enemy_->GetTranslation();

	Vector3 playerPos = enemy_->GetPlayer()->GetWorldPosition();

	// 自機と敵の距離を計算
	float distanceZ = pos.z - playerPos.z;

	// 方向とスピード
	Vector3 dir = enemy_->GetMoveDirection();
	float speed = enemy_->GetMoveSpeed();


	switch (phase_) {
	case Phase::Approach: {

		enemy_->Move({ dir.x * speed, dir.y * speed, dir.z * speed });

		// 滞空フェーズへ
		if (distanceZ <= 45.0f) {
			phase_ = Phase::Hold;
		}
		break;
	}

	case Phase::Hold: {
		// 滞空
		time_ += 0.05f;
		float moveX = std::cos(time_) * 0.15f;

		float targetZ = playerPos.z + 45.0f; // 目標位置
		float moveZ = targetZ - pos.z;      
		enemy_->Move({ moveX, 0.0f, moveZ });

		break;
	}

	case Phase::Leave: {
		// 離脱

		float leaveSpeed = speed + 0.2f;
		enemy_->Move({ dir.x * leaveSpeed, dir.y * leaveSpeed, dir.z * leaveSpeed });

		// 自機の後ろになったら完全に離脱
		if (distanceZ < -5.0f) {
			enemy_->ClearTimedCalls();
			enemy_->ChangeState(new EnemyStateLeave());
		}
		break;
	}
	}
}