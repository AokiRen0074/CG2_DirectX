#include "PlayerBullet.h"
#include <cassert>
#include "CollisionConfig.h"
#include "BaseEnemy.h"
#include "Audio/Audio.h"

// 音
static bool sIsHitSoundLoaded = false;
static SoundData sHitSound = {};

// 初期化
void PlayerBullet::Initialize(NeonModel* model, const Vector3& position, const Vector3& velocity, const Vector3& rotation) {
	// Nullポインタチェック
	assert(model);
	neonModel_ = model;
	velocity_ = velocity;

	static uint32_t sSharedTextureHandle = TextureManager::Load("Resources/ring.png");
	textureHandle_ = sSharedTextureHandle;

	if (!isTransformInitialized_) {
		worldTransform_.Initialize();
		for (int i = 0; i < kMaxTrail; ++i) {
			trailTransforms_[i].Initialize();
		}
		isTransformInitialized_ = true;
	}

	// 引数で受け取った初期座標をセット
	worldTransform_.translation_ = position;
	worldTransform_.scale_ = { 0.2f, 0.2f, 0.2f };
	worldTransform_.rotation_ = rotation;

	// 自分の属性をプレイヤーに設定
	SetCollisionAttribute(kCollisionAttributePlayer);
	// 当たる相手をプレイヤー以外に設定
	SetCollisionMask(~kCollisionAttributePlayer);

	// 変数の初期化
	target_ = nullptr;
	prevTarget_ = nullptr;
	lockOnAnimTimer_ = 0;
	trailHistory_.clear(); 

	isDead_ = false;
	deathTimer_ = 60;


	if (lockOnSprite_ == nullptr) {
		uint32_t lockOnTex = TextureManager::Load("Resources/lockon.png");
		lockOnSprite_ = Sprite::Create(lockOnTex, { 0, 0 });
	}

	if (!sIsHitSoundLoaded) {
		sHitSound = Audio::GetInstance()->SoundLoadWave("Sounds/hit.wav");
		sIsHitSoundLoaded = true;
	}

}

/*----------------------------------
衝突時コールバック
-----------------------------*/
void PlayerBullet::OnCollision() {

	if (!isDead_) {
		Audio::GetInstance()->SoundPlayWave(sHitSound);
	}

	isDead_ = true;


}

// 更新処理
void PlayerBullet::Update(const std::list<BaseEnemy*>& enemies) {

	if (isDead_) return;

	// 時間経過で消す
	if (--deathTimer_ <= 0) {
		isDead_ = true;
	}

	bool isTargetValid = false;
	if (target_) {
		for (BaseEnemy* enemy : enemies) {
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

		for (BaseEnemy* enemy : enemies) {
			if (enemy->IsDead()) continue;

			if (enemy->IsObstacle()) continue;

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
		const float kBulletSpeed = 4.0f;

		Vector3 toEnemy = target_->GetWorldPosition() - GetWorldPosition();
		float lenToEnemy = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.y * toEnemy.y + toEnemy.z * toEnemy.z);

		float kHomingInterpolation = 0.05f; // 遠い時はふんわり曲がる
		if (lenToEnemy < 100.0f) {
			kHomingInterpolation = 0.2f;    // 射程圏内に入ったら急カーブ
		}
		if (lenToEnemy < 50.0f) {
			kHomingInterpolation = 1.0f;    
		}

		if (lenToEnemy <= kBulletSpeed * 2.0f) {
			worldTransform_.translation_ = target_->GetWorldPosition(); // 敵の位置にワープ
		
			if (!isDead_) {
				Audio::GetInstance()->SoundPlayWave(sHitSound);
			}
			
			isDead_ = true; // 当たった扱いにして消滅させる

		target_->OnCollision(); 
		}
		else {
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

			// 球面線形補間で、敵の方向へベクトルを曲げる
			Vector3 slerpVelocity = Slerp(velocity_, toEnemy, kHomingInterpolation);
			velocity_.x = slerpVelocity.x * kBulletSpeed;
			velocity_.y = slerpVelocity.y * kBulletSpeed;
			velocity_.z = slerpVelocity.z * kBulletSpeed;

			// 弾の「見た目（回転）」も進行方向に合わせる
			worldTransform_.rotation_.y = std::atan2(velocity_.x, velocity_.z);
			float xzLength = std::sqrt(velocity_.x * velocity_.x + velocity_.z * velocity_.z);
			worldTransform_.rotation_.x = std::atan2(-velocity_.y, xzLength);
		}
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
	
	/*------------------------
	UI表示
	---------------------------*/
	if (target_ != prevTarget_) {
		if (target_ != nullptr) {
			lockOnAnimTimer_ = 15; 
		}
		prevTarget_ = target_;
	}

	if (lockOnAnimTimer_ > 0) {
		lockOnAnimTimer_--;
	}

	// ワールドトランスフォームの更新
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	worldTransform_.TransferMatrix();
}

void PlayerBullet::Draw(const ViewProjection& viewProjection) {

	if (isDead_) return;

	if (neonModel_) {

		// ==========================================
		// 色のグラデーション設定
		// ==========================================
		// 先端の色（水色）
		Vector3 headColor = { 0.0f, 0.8f, 1.0f };
		// 尻尾の色（ピンクや紫）
		Vector3 tailColor = { 1.0f, 0.0f, 0.8f };

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

			// 光の強さを徐々に暗くする
			float intensity = 15.0f * ratio;

			// 色のグラデーション計算（尻尾の色から先端の色へ滑らかに混ぜる魔法！）
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

void PlayerBullet::DrawUI(const ViewProjection& viewProjection) {
	if (isDead_ || !target_ || !lockOnSprite_) return;

	// 敵の3D座標を、画面の2D座標に変換する
 	Vector3 p = target_->GetWorldPosition();
	Matrix4x4 matVP = Multiply(viewProjection.matView, viewProjection.matProjection);
	float w = p.x * matVP.m[0][3] + p.y * matVP.m[1][3] + p.z * matVP.m[2][3] + matVP.m[3][3];

	if (w > 0.1f) {
		float nx = (p.x * matVP.m[0][0] + p.y * matVP.m[1][0] + p.z * matVP.m[2][0] + matVP.m[3][0]) / w;
		float ny = (p.x * matVP.m[0][1] + p.y * matVP.m[1][1] + p.z * matVP.m[2][1] + matVP.m[3][1]) / w;

		float screenX = (nx + 1.0f) * 0.5f * 1280.0f;
		float screenY = (1.0f - ny) * 0.5f * 720.0f;

		// アニメーション計算
		float ratio = (float)lockOnAnimTimer_ / 15.0f;
		float easeRatio = ratio * ratio;

		float baseScale = 0.5f; 
		float scale = baseScale + (baseScale * 2.0f * easeRatio);
		float rotation = 3.141592f * 2.0f * easeRatio;

		// Spriteに直接セットして描画！
		lockOnSprite_->SetPosition({ screenX, screenY });
		lockOnSprite_->GetTransform().scale = { 128.0f * scale, 128.0f * scale, 1.0f };
		lockOnSprite_->GetTransform().rotate.z = rotation;

		lockOnSprite_->Update();
		lockOnSprite_->Draw();
	}
}