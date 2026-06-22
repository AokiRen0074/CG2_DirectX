#include "PlayerBullet.h"
#include <cassert>

// 初期化
void PlayerBullet::Initialize(Object3d* model, const Vector3& position) {

	// Nullポインタチェック
	assert(model);

	model_ = model;

	// テクスチャ読み込み
	textureHandle_ = TextureManager::Load("Resources/ring.png");

	worldTransform_.scale_ = { 0.2f, 0.2f, 0.2f };

	worldTransform_.Initialize();

	// 引数で受け取った初期座標をセット
	worldTransform_.translation_ =position;
}

// 更新処理
void PlayerBullet::Update() {


	// ワールドトランスフォームの更新
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	worldTransform_.TransferMatrix();
}


