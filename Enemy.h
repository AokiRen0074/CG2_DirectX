#pragma once
#include "WorldTransform.h"
#include "Object3d.h"
#include "TextureManager.h"

class Enemy {
public:
	// 初期化
	void Initialize(Object3d* model, uint32_t textureHandle);

	// 更新処理
	void Update();

	// 描画処理
	void Draw(const ViewProjection& viewProjection);

private:

	WorldTransform worldTransform_;

	// モデル
	Object3d* model_ = nullptr;

	// テクスチャハンドル
	uint32_t textureHandle_ = 0u;

};