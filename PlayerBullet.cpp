#include "PlayerBullet.h"
#include <cassert>
#include "CollisionConfig.h"

// 初期化
void PlayerBullet::Initialize(NeonModel* model, const Vector3& position, const Vector3& velocity, const Vector3& rotation) {
	// Nullポインタチェック
	assert(model);

	neonModel_ = model;

	velocity_ = velocity;

	// テクスチャ読み込み
	textureHandle_ = TextureManager::Load("Resources/ring.png");


	worldTransform_.Initialize();

	// 引数で受け取った初期座標をセット
	worldTransform_.translation_ =position;


	worldTransform_.scale_ = { 0.2f, 0.2f, 0.2f };

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
void PlayerBullet::Update() {

	// 時間経過で消す
	if (--deathTimer_ <= 0) {
		isDead_ = true;
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
		// ✨ 色のグラデーション設定
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

