#pragma once
#include "BaseEnemy.h"

class WeakEnemyTriangle : public BaseEnemy {
public:
	static void StaticInitialize();
	void Initialize(Player* player) override;
	void DrawNeon(const ViewProjection& viewProjection) override;
	void DrawImGui() override;

private:
	void ApplyGlobalVariables();

	// 三角形ネオンのモデル
	static NeonModel* sNeonTriangle;

	// 種族で共有するマスターカラー
	static Vector3 sTriangleColor;
	static float sTriangleIntensity;


};