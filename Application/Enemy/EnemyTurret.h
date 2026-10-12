#pragma once
#include "BaseEnemy.h"
#include "EnemyLaserBeam.h"

class EnemyTurret : public BaseEnemy {
public:
	void Initialize(Player* player) override;
	void Update() override;
	void DrawNeon(const ViewProjection& viewProjection) override;
	void OnCollision() override;
	void SetLaser(EnemyLaserBeam* laser) { laser_ = laser; }

private:
	EnemyLaserBeam* laser_ = nullptr;
	NeonModel* turretBaseModel_ = nullptr;
	NeonModel* turretGunModel_ = nullptr;
	WorldTransform transformGun_;
	int attackTimer_ = 0;
};