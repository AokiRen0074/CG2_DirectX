#include "EnemyBullet.h"
#include "cassert"
#include "TextureManager.h"

void EnemyBullet::Initialize(Object3d* model, const Vector3 position, const Vector3& velocity){
	assert(model);

	model_ = model;

	velocity_ = velocity;

	// モデル読み込み
	//textureHandle_ = TextureManager::Load("Resources/block.png");

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();

	worldTransform_.scale_ = { 0.5f, 0.5f, 5.0f };

	// 引数で受け取った初期座標をセット
	worldTransform_.translation_ = position;

	worldTransform_.rotation_.y = std::atan2(velocity.x, velocity.z);

	// 横軸方向の長さを求める
	float xzLength = std::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);

	// X軸周り角度
	worldTransform_.rotation_.x = std::atan2(-velocity.y, xzLength);


	// 行列の更新
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

}

void EnemyBullet::Update() {

	worldTransform_.translation_.x += velocity_.x;
	worldTransform_.translation_.y += velocity_.y;
	worldTransform_.translation_.z += velocity_.z;

	// 時間経過でデス
	if (--deathTimer_ <= 0) {
		isDead_ = true;
	}

	// ワールドトランスフォームの更新
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	worldTransform_.TransferMatrix();
}

void EnemyBullet::Draw(const ViewProjection& camera) {
	
	model_->Draw(worldTransform_, camera, textureHandle_);
}