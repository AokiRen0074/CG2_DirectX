#include "PlayerBullet.h"
#include <cassert>
#include "CollisionConfig.h"
#include "Enemy.h"

// 初期化
void PlayerBullet::Initialize(NeonModel* model, const Vector3& position, const Vector3& velocity, const Vector3& rotation) {
	// Nullポインタチェック
	assert(model);

	neonModel_ = model;

	velocity_ = velocity;

	// テクスチャ読み込み
	static uint32_t sSharedTextureHandle = TextureManager::Load("Resources/ring.png");
	textureHandle_ = sSharedTextureHandle;


	worldTransform_.Initialize();

	// 引数で受け取った初期座標をセット
	worldTransform_.translation_ =position;


	worldTransform_.scale_ = { 0.2f, 0.2f, 0.2f };

	worldTransform_.rotation_ = rotation;

	for (int i = 0; i < kMaxTrail; ++i) {
		trailTransforms_[i].Initialize();
	}

	// 自分の属性をプレイヤーに設定
	SetCollisionAttribute(kCollisionAttributePlayer);
	// 当たる相手をプレイヤー以外」に設定
	SetCollisionMask(~kCollisionAttributePlayer);
}

/*----------------------------------
衝突時コールバック
-----------------------------*/
void PlayerBullet::OnCollision() {
	isDead_ = true;
}

// 更新処理
void PlayerBullet::Update(const std::list<Enemy*>& enemies) {

	// 時間経過で消す
	if (--deathTimer_ <= 0) {
		isDead_ = true;
	}

	bool isTargetValid = false;
	if (target_) {
		for (Enemy* enemy : enemies) {
			// 最新の敵リストの中に自分のターゲットがまだいて、かつ死んでいなければOK
			if (enemy == target_ && !enemy->IsDead()) {
				isTargetValid = true;
				break;
			}
		}
	}


	if (!isTargetValid) {
		target_ = nullptr;
		float closestDist = 999999.0f; // 十分に大きな値で初期化

		for (Enemy* enemy : enemies) {
			if (enemy->IsDead()) continue;

			Vector3 toEnemy = enemy->GetWorldPosition() - GetWorldPosition();
			float dist = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);

			// 弾から 250.0f 以内にいる一番近い敵をロックオン
			if (dist < closestDist && dist < 250.0f) {
				closestDist = dist;
				target_ = enemy;
			}
		}
	}

	// ターゲットがいるなら、そちらへ曲がる
	if (target_) {
		const float kBulletSpeed = 2.0f;       // ミサイルの速さ
		const float kHomingInterpolation = 0.15f; // 曲がる強さ

		Vector3 toEnemy = target_->GetWorldPosition() - GetWorldPosition();
		float lenToEnemy = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);
		if (lenToEnemy != 0.0f) {
			toEnemy.x /= lenToEnemy;
			toEnemy.y /= lenToEnemy;
			toEnemy.z /= lenToEnemy;
		}

		float lenVelocity = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
		if (lenVelocity != 0.0f) {
			velocity_.x /= lenVelocity;
			velocity_.y /= lenVelocity;
			velocity_.z /= lenVelocity;
		}

		// 球面線形補間で、敵の方向へ少しずつベクトルを曲げる
		Vector3 slerpVelocity = Slerp(velocity_, toEnemy, kHomingInterpolation);
		velocity_.x = slerpVelocity.x * kBulletSpeed;
		velocity_.y = slerpVelocity.y * kBulletSpeed;
		velocity_.z = slerpVelocity.z * kBulletSpeed;

		// 弾の「見た目（回転）」も進行方向に合わせる
		worldTransform_.rotation_.y = std::atan2(velocity_.x, velocity_.z);
		float xzLength = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
		worldTransform_.rotation_.x = std::atan2(-velocity_.y, xzLength);
	}

	// 座標を移動させる
	worldTransform_.translation_.x += velocity_.x;
	worldTransform_.translation_.y += velocity_.y;
	worldTransform_.translation_.z += velocity_.z;

	trailHistory_.push_front(worldTransform_.translation_);
	// 長くなりすぎたら一番古い尻尾を消す
	if (trailHistory_.size() > kMaxTrail) {
		trailHistory_.pop_back();
	}

	// ワールドトランスフォームの更新
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	worldTransform_.TransferMatrix();
}

void PlayerBullet::Draw(const ViewProjection& viewProjection) {
	if (neonModel_) {

		// ==========================================
		// 色のグラデーション設定
		// ==========================================
		// 先端の色（水色）
		Vector3 headColor = { 0.0f, 0.8f, 1.0f };
		// 尻尾の色（ピンクや紫）
		Vector3 tailColor = { 1.0f, 0.0f, 0.8f };

		// 1. 「弾本体（頭）」の描画（一番強くて水色）
		neonModel_->SetNeonColor(15.0f, headColor.x, headColor.y, headColor.z);
		neonModel_->Draw(worldTransform_, viewProjection, textureHandle_);

		// 2. 「3Dの軌道（尾）」の描画
		int index = 0;
		for (const Vector3& pos : trailHistory_) {
			// 先頭はスキップ
			if (index == 0) { index++; continue; }
			if (index >= kMaxTrail) break;

			// ratio(割合) は 先端(1.0) から 尻尾(0.0) へ近づく
			float ratio = 1.0f - ((float)index / trailHistory_.size());

			trailTransforms_[index].translation_ = pos;
			trailTransforms_[index].rotation_ = worldTransform_.rotation_;

			// サイズを小さくする
			float scale = 0.2f * ratio;
			trailTransforms_[index].scale_ = { scale, scale, scale };

			trailTransforms_[index].matWorld_ = MakeAffineMatrix(
				trailTransforms_[index].scale_,
				trailTransforms_[index].rotation_,
				trailTransforms_[index].translation_
			);
			trailTransforms_[index].TransferMatrix();

			// ✨ 光の強さを徐々に暗くする
			float intensity = 15.0f * ratio;

			// ✨ 色のグラデーション計算（尻尾の色から先端の色へ滑らかに混ぜる魔法！）
			float r = tailColor.x + (headColor.x - tailColor.x) * ratio;
			float g = tailColor.y + (headColor.y - tailColor.y) * ratio;
			float b = tailColor.z + (headColor.z - tailColor.z) * ratio;

			neonModel_->SetNeonColor(intensity, r, g, b);

			// 描画！
			neonModel_->Draw(trailTransforms_[index], viewProjection, textureHandle_);

			index++;
		}
	}
}
Vector3 PlayerBullet::GetWorldPosition() {
	Vector3 worldPos;
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

