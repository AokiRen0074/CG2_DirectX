#pragma once
#include "BaseEnemy.h"

class WeakEnemySpinCore : public BaseEnemy {
public:
	static void StaticInitialize();
	void Initialize(Player* player) override;

	// 更新処理
	void Update() override;

	void DrawNeon(const ViewProjection& viewProjection) override;
	void DrawImGui() override;

private:
	void ApplyGlobalVariables();

	// すべてネオン
	static NeonModel* sNeonFrame; // 回転しない外枠用
	static NeonModel* sNeonCore;  // 回転するコア用

	// コア専用の回転・座標計算
	WorldTransform transformCore_;

	static Vector3 sSpinCoreColor;
	static float sSpinCoreIntensity;
};