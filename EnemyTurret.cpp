#include "EnemyTurret.h"
#include "GameScene.h"
#include <cmath>

void EnemyTurret::Initialize(Player* player) {
	BaseEnemy::Initialize(player);

	turretBaseModel_ = new NeonModel();
	turretBaseModel_->Initialize("Resources/Enemy/Neon", "Turret_Base.obj");

	turretGunModel_ = new NeonModel();
	turretGunModel_->Initialize("Resources/Enemy/Neon", "Turret_Gun.obj");

	transformGun_.Initialize();
	attackTimer_ = 0;
	SetRadius(3.0f); // 砲台の当たり判定
}

void EnemyTurret::Update() {
	attackTimer_++;

	if (laser_ && laser_->IsDead()) {
		isDead_ = true;
		if (gameScene_->GetParticleManager()) {
			gameScene_->GetParticleManager()->EmitStar(GetWorldPosition(), 40, { 1.0f, 0.2f, 0.0f });
		}
		return;
	}

	if (player_) {
		worldTransform_.translation_.z = player_->GetWorldPosition().z;
	}
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	int cycle = attackTimer_ % 240;
	bool isAiming = (cycle < 120);
	bool isFiring = (cycle >= 120 && cycle < 160);

	if (isAiming && player_) {
		// 狙い中のみ、砲身が自機を追いかける
		Vector3 pPos = player_->GetWorldPosition();
		Vector3 tPos = GetWorldPosition();
		float dx = pPos.x - tPos.x;
		float dy = pPos.y - tPos.y;

		float targetAngleZ = std::atan2(dy, dx);
		float diff = targetAngleZ - transformGun_.rotation_.z;

		while (diff > 3.14159f) diff -= 6.28318f;
		while (diff < -3.14159f) diff += 6.28318f;
		transformGun_.rotation_.z += diff * 0.05f;
	}

	transformGun_.translation_ = GetWorldPosition();
	transformGun_.matWorld_ = MakeAffineMatrix(transformGun_.scale_, transformGun_.rotation_, transformGun_.translation_);

	if (laser_ && !laser_->IsDead()) {
		// 状態をレーザーに伝達
		laser_->SetState(isAiming, isFiring);
		laser_->SetBeamTransform(transformGun_.translation_, transformGun_.rotation_, 150.0f);
	}

	if (isFiring && gameScene_->GetParticleManager() && (attackTimer_ % 2 == 0)) {
		gameScene_->GetParticleManager()->EmitStar(transformGun_.translation_, 3, { 1.0f, 0.1f, 0.0f });
	}
}

void EnemyTurret::DrawNeon(const ViewProjection& viewProjection) {
	if (turretBaseModel_) {
		turretBaseModel_->SetNeonColor(5.0f, 1.0f, 0.0f, 0.0f); // 真っ赤な砲台
		turretBaseModel_->Draw(worldTransform_, viewProjection, dummyTexture_);
	}
	if (turretGunModel_) {
		turretGunModel_->SetNeonColor(8.0f, 1.0f, 0.2f, 0.0f); // 少しオレンジの砲身
		turretGunModel_->Draw(transformGun_, viewProjection, dummyTexture_);
	}
}

void EnemyTurret::OnCollision() {
	if (player_) {
		Vector3 pPos = player_->GetWorldPosition();
		Vector3 tPos = GetWorldPosition();
		float dx = pPos.x - tPos.x;
		float dy = pPos.y - tPos.y;
		float dz = pPos.z - tPos.z;
		float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

		// 砲台と自機が触れている距離ならスキップ
		if (dist <= GetRadius() + 1.5f) {
			return;
		}
	}

	if (gameScene_ && gameScene_->GetParticleManager()) {
		gameScene_->GetParticleManager()->EmitStar(GetWorldPosition(), 5, { 1.0f, 1.0f, 0.5f });
	}

	// ダメージ処理はレーザーに丸投げ
	if (laser_ && !laser_->IsDead()) {
		laser_->OnCollision();
	}
}