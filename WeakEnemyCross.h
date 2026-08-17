#pragma once
#include "BaseEnemy.h"

class WeakEnemyCross : public BaseEnemy {
public:
	// 新しい敵専用のモデル読み込み
	static void StaticInitialize();

	// 初期化
	void Initialize(Player* player) override;

	// 描画
	void DrawNeon(const ViewProjection& viewProjection) override;

	void DrawImGui() override;

private:

	// jsonからデータを読み込む
	void ApplyGlobalVariables();	

	// この敵専用の静的モデル
	static BodyModel* sBodyModelCross;
	static NeonModel* sNeonModelCross;
	static NeonModel* sNeonPatternCross; 

	// インスタンスが持つ3つ目のモデルポインタ
	NeonModel* modelPattern_ = nullptr;
};