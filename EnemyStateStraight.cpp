#include "EnemyStateStraight.h"
#include "BaseEnemy.h"
#include "Application/Character/Player.h"

void EnemyStateStraight::Update() {
	Vector3 pos = enemy_->GetTranslation();
	Vector3 playerPos = enemy_->GetPlayer()->GetWorldPosition();

	// マスター設定された方向とスピードを取得
	Vector3 dir = enemy_->GetMoveDirection();
	float speed = enemy_->GetMoveSpeed();

	enemy_->Move({ dir.x * speed, dir.y * speed, dir.z * speed });

	//  画面の奥まで通り過ぎたら消滅させる
	if (pos.z - playerPos.z < -20.0f) {
	
		enemy_->IsDead();
	}
}