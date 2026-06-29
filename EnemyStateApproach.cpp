#include "EnemyStateApproach.h"
#include "Enemy.h"
#include "EnemyStateLeave.h"

void EnemyStateApproach::Update() {
	enemy_->Move(approachVelocity_);

	enemy_->UpdateFireTimer();

	// 離脱フェーズへ遷移
	if (enemy_->GetTranslation().z < 0.0f) {
		enemy_->ChangeState(new EnemyStateLeave());
	}
}