#include "EnemyStateApproach.h"
#include "BaseEnemy.h"
#include "EnemyStateLeave.h"

void EnemyStateApproach::Update() {
	enemy_->Move(approachVelocity_);


	// 離脱フェーズへ遷移
	if (enemy_->GetTranslation().z < 0.0f) {

		enemy_->ClearTimedCalls();
		enemy_->ChangeState(new EnemyStateLeave());
	}
}