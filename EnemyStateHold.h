#pragma once
#include "BaseEnemyState.h"

class EnemyStateHold : public BaseEnemyState {
public:
	void Update() override;

private:
	// 敵の行動フェーズ
	enum class Phase {
		Approach, // 奥から猛スピードで登場
		Hold,     // プレイヤーの前方で滞空・横揺れ
		Leave     // プレイヤーの横を猛スピードですり抜ける
	};

	Phase phase_ = Phase::Approach;

	// 滞空時間を計るタイマー
	int holdTimer_ = 180; // 3秒間

	// 横揺れのための時間変数
	float time_ = 0.0f;
};