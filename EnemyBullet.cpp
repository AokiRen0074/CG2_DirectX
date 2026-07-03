#include "EnemyBullet.h"
#include "cassert"
#include "TextureManager.h"
#include "Application/Character/Player.h"

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

/*---------------------------------------
衝突時コールバック
------------------------------------------*/
void EnemyBullet::OnCollision() {
	isDead_ = true;
}


void EnemyBullet::Update() {

	// ホーミング
	if (player_) {
		const float kBulletSpeed = 1.0f; // 敵弾の速さ
		const float kHomingInterpolation = 0.05f; // 1フレームでの補間割合 
		// 敵弾から自キャラへのベクトルを計算
		Vector3 toPlayer;
		Vector3 playerPos = player_->GetworldPosition();
		toPlayer.x = playerPos.x - worldTransform_.translation_.x;
		toPlayer.y = playerPos.y - worldTransform_.translation_.y;
		toPlayer.z = playerPos.z - worldTransform_.translation_.z;

		// ベクトルを正規化する
		float lengthToPlayer = std::sqrt(toPlayer.x * toPlayer.x + toPlayer.y * toPlayer.y + toPlayer.z * toPlayer.z);
		if (lengthToPlayer != 0.0f) {
			toPlayer.x /= lengthToPlayer;
			toPlayer.y /= lengthToPlayer;
			toPlayer.z /= lengthToPlayer;
		}

		float lengthVelocity = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
		if (lengthVelocity != 0.0f) {
			velocity_.x /= lengthVelocity;
			velocity_.y /= lengthVelocity;
			velocity_.z /= lengthVelocity;
		}

		// 球面線形補間により、新たな速度とする
		Vector3 slerpVelocity = Slerp(velocity_, toPlayer, kHomingInterpolation);
		velocity_.x = slerpVelocity.x * kBulletSpeed;
		velocity_.y = slerpVelocity.y * kBulletSpeed;
		velocity_.z = slerpVelocity.z * kBulletSpeed;

		// 進行方向に見た目の回転を合わせる
		worldTransform_.rotation_.y = std::atan2(velocity_.x, velocity_.z);
		float xzLength = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
		worldTransform_.rotation_.x = std::atan2(-velocity_.y, xzLength);
	}

	// 座標の更新
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

/*-----------------------------
敵弾のワールド座標
--------------------------------*/
Vector3 EnemyBullet::GetWorldBulletPosition() {
	Vector3 worldPos;
	// ワールド座標の平行移動成分を取得
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];

	return worldPos;
}