#pragma once
#include "Application/Character//Player/Player.h"
#include "Application/Character/Enemy/EnemyManager.h"
#include "Input/Input.h"

class RhythmJudge {
public:
    void Initialize();

    // プレイヤーと敵マネージャーを受け取って判定を行う
    void Update(Player* player, EnemyManager* enemyManager);

private:
    // 判定の定数
    const float kHitRangeX_ = 2.0f;      // 叩けるX座標の範囲
    const float kLaneThresholdY_ = 1.0f; // 上下レーンを区別するYの境界線
};