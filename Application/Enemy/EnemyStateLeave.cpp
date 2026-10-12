#include "EnemyStateLeave.h"
#include "BaseEnemy.h"

void EnemyStateLeave::Update() {
	// EnemyのMove関数を使って移動させる
	enemy_->Move(leaveVelocity_);

}