#pragma once
#include "Model.h"
#include "WorldTransform.h"


class Player {
public:

	// 初期化
	void Initialize(ModelData* model, uint32_t textureHandle);

	// 更新処理
	void Update();

	// 描画処理
	void Draw();


private:

	WorldTransform worldTransform_;

	// モデル
	ModelData *model_ = nullptr;

	// テクスチャハンドル
	uint32_t textureHandle_ = 0u;


};