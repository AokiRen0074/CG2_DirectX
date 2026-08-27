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

	// プレイヤーとの距離の差分を計算
	float diffX = pos.x - playerPos.x;
	float diffY = pos.y - playerPos.y;
	float diffZ = pos.z - playerPos.z;


	if (diffZ < -20.0f || 
		diffX < -80.0f || diffX > 80.0f || // X軸：左右の画面外に消えた
		diffY < -40.0f || diffY > 40.0f) {  // Y軸：上下の画面外に消えた

		// 完全に画面外に出たので、強制的に消去！
		enemy_->Kill();
	}


}