#pragma once

class Enemy;

class BaseEnemyState {
protected:

	// 親クラスでEnemyポインタを持たせる
	Enemy* enemy_ = nullptr;

public:
	virtual ~BaseEnemyState() = default;

	virtual void Update() = 0;

	// Enemyをセットする関数
	void SetEnemy(Enemy* enemy) { enemy_ = enemy; }
};
