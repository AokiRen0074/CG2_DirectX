#include "EnemyBullet.h"
#include "cassert"
#include "TextureManager.h"
#include "CollisionConfig.h"
#include <numbers>
#include "Application/Character/Player.h"

void EnemyBullet::Initialize(NeonModel* model, const Vector3 position, const Vector3& velocity, uint32_t textureHandle) {
	assert(model);

	model_ = model;

	velocity_ = velocity;

	textureHandle_ = textureHandle;

	// モデル読み込み
	//textureHandle_ = TextureManager::Load("Resources/block.png");

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();

	worldTransform_.scale_ = { 0.3f, 0.3f, 10.5f };

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

	// 自分の属性を敵に設定
	SetCollisionAttribute(kCollisionAttributeEnemy);
	//当たる相手を敵に設定
	SetCollisionMask(~kCollisionAttributeEnemy);

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
		Vector3 playerPos = player_->GetWorldPosition();
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

	colorTimer_ += 0.05f;


	// ワールドトランスフォームの更新
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	worldTransform_.TransferMatrix();
}

void EnemyBullet::Draw(const ViewProjection& camera) {
	if (model_) {
		const float pi = std::numbers::pi_v<float>;


		float r = std::sin(colorTimer_) * 0.5f + 0.5f;
		float g = std::sin(colorTimer_ + (2.0f * pi / 3.0f)) * 0.5f + 0.5f; // 120度ズラす
		float b = std::sin(colorTimer_ + (4.0f * pi / 3.0f)) * 0.5f + 0.5f; // 240度ズラす

		// 強烈に光らせる
		float intensity = 15.0f;

		// ネオンカラーをセットして描画
		model_->SetNeonColor(intensity, r, g, b);
		model_->Draw(worldTransform_, camera, textureHandle_);
	}
}

/*-----------------------------
敵弾のワールド座標
--------------------------------*/
Vector3 EnemyBullet::GetWorldPosition() {
	Vector3 worldPos;
	// ワールド座標の平行移動成分を取得
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];

	return worldPos;
}