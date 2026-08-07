#pragma once


class BaseEnemy;

class BaseEnemyState {
protected:

	// 親クラスでEnemyポインタを持たせる
	BaseEnemy* enemy_ = nullptr;

public:
	virtual ~BaseEnemyState() = default;

	virtual void Update() = 0;

	// Enemyをセットする関数
	void SetEnemy(BaseEnemy* enemy) { enemy_ = enemy; }
};
