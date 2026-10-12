#pragma once
#include "WorldTransform.h"
#include "Object3d.h"
#include "ViewProjection.h" 


class Skydome {

public:

	void Initialize(Object3d* model,uint32_t textureHandle);

	// 更新処理
	void Update();

	void Draw(const ViewProjection& viewProjection);

private:

	// ワールド変換データ
	WorldTransform worldTransform_;

	// テクスチャハンドル
	uint32_t textureHandle_ = 0u;

	// モデル
	Object3d* model_ = nullptr;
};