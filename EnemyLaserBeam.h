#pragma once
#include "BaseEnemy.h"

class EnemyLaserBeam : public BaseEnemy {
public:
	void Initialize(Player* player) override;
	void Update() override;
	void DrawNeon(const ViewProjection& viewProjection) override;
	void OnCollision() override;
	float GetDistanceTo(const Vector3& targetPos) override;

	void SetBeamTransform(const Vector3& start, const Vector3& angles, float length);
	void SetIsFiring(bool firing) { isFiring_ = firing; }


	void DrawImGui() override;

	void SetState(bool isAiming, bool isFiring) {
		isAiming_ = isAiming;
		isFiring_ = isFiring;
	}

	void Kill() { isDead_ = true; }

private:
	bool isAiming_ = false;
	bool isFiring_ = false;
	WorldTransform transformAura_;
	WorldTransform transformCore_;

	WorldTransform transformAim_;


	uint32_t whiteTexture_ = 0u;

	// レーザーの体力
	int maxHp_ = 3;
	int currentHp_ = 3;

	// 多段ヒットを防ぐ
	int hitTimer_ = 0;
	int flashTimer_ = 0;

	Vector3 startPos_;
};