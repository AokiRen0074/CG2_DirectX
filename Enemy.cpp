#include "Enemy.h"
#include "EnemyStateApproach.h"
#include <cassert>


#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

// デストラクタ
Enemy::~Enemy() {
	for (EnemyBullet* bullet : bullets_) {
		delete bullet;

	}
}

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

	// 最初の状態
	state_ = new EnemyStateApproach();
	state_->SetEnemy(this);

	ApproachPhaseInitialize();

}

// 接近フェーズ初期化
void Enemy::ApproachPhaseInitialize() {
	// 発射タイマーを初期化
	fireTimer = kFireInterval;

}

void Enemy::UpdateFireTimer() {
	// 発射タイマーカウントダウン
	fireTimer--;

	// 指定時間に達した
	if (fireTimer <= 0) {
		// 弾を発射
		Fire();
		// 発射タイマーを初期化
		fireTimer = kFireInterval;
	}
}

void Enemy::Fire() {


	// 弾の速度
	const float kBulletSpeed = 1.0f;
	Vector3 velocity(0, 0, kBulletSpeed);

	EnemyBullet* newBullet = new EnemyBullet();
	newBullet->Initialize(model_, worldTransform_.translation_, velocity);

	// 弾を登録する
	bullets_.push_back(newBullet);




}


/*--------------------------
更新処理
------------------------------------*/
void Enemy::Update() {


	for (EnemyBullet* bullet : bullets_) {
		bullet->Update();
	}
	// 状態遷移
	if (state_) {
		state_->Update();
	}

	// デスフラグの立った弾を削除
	bullets_.remove_if([](EnemyBullet* bullet) {
		if (bullet->IsDead()) {
			delete bullet;
			return true;
		}
		return false;
		});



	// 行列の更新
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	worldTransform_.TransferMatrix();





#ifdef USE_IMGUI

	// キャラクターの座標を画面表示する処理

	ImGui::Begin("Enemy");

	ImGui::Text("Position: X: %f, Y: %f, Z: %f",
		worldTransform_.translation_.x,
		worldTransform_.translation_.y,
		worldTransform_.translation_.z);

	ImGui::End();

#endif
}


// 描画処理
void Enemy::Draw(const ViewProjection& viewProjection) {

	// 敵の描画
	model_->Draw(worldTransform_, viewProjection, textureHandle_);

	// 弾の描画
	for (EnemyBullet* bullet : bullets_) {
		bullet->Draw(viewProjection);
	}

}

// 状態を切り替える関数
void Enemy::ChangeState(BaseEnemyState* newState) {
	// いあの状態を消して、新しい状態を入れる
	if (state_) {
		delete state_;
	}

	state_ = newState;
	state_->SetEnemy(this);
}

// 移動関数
void Enemy::Move(const Vector3& velocity) {
	worldTransform_.translation_.x += velocity.x;
	worldTransform_.translation_.y += velocity.y;
	worldTransform_.translation_.z += velocity.z;
}

// 座標のゲッター
Vector3 Enemy::GetTranslation() const {
	return worldTransform_.translation_;
}