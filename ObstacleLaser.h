#pragma once
#include "BaseEnemy.h"
#include "Application/Character/Player.h"

//  壊れない・ロックオンされないレーザー障害物
class ObstacleLaser : public BaseEnemy {
public:
	void Initialize(Player* player) override;
	void Update() override;
	void DrawNeon(const ViewProjection& viewProjection) override;
	void DrawImGui() override;

	void OnCollision() override;

	float GetDistanceTo(const Vector3& targetPos) override;

private:
	uint32_t whiteTexture_ = 0;

};
