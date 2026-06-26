#include "Enemy.h"
#include <cassert>

void Enemy::Initialize(Object3d* model, uint32_t textureHandle) {

	assert(model);

	model_ = model;

	textureHandle_ = textureHandle;

	worldTransform_.Initialize();

	// テクスチャ読み込み
	//textureHandle_ = TextureManager::Load("Resources/monsterBall.png");

	// 初期座標
	worldTransform_.scale_ = { 1.0f, 1.0f, 1.0f };
	worldTransform_.rotation_ = { 0.0f, 0.0f, 0.0f };
	worldTransform_.translation_ = { 0.0f, 0.0f, 50.0f };

}

// 更新処理
void Enemy::Update() {

	// 速度
	Vector3 velocity = { 0.0f, 0.0f, 0.1f };

	// 敵の移動処理
	worldTransform_.translation_.z -= velocity.z;

	// 行列の更新
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	worldTransform_.TransferMatrix();
}


// 描画処理
void Enemy::Draw(const ViewProjection& viewProjection) {

	// 敵の描画
	model_->Draw(worldTransform_, viewProjection, textureHandle_);


}