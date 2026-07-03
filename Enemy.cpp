#include "Enemy.h"
#include "EnemyStateApproach.h"
#include <cassert>
#include "cmath"
#include "Application/Character/Player.h"


#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

// デストラクタ
Enemy::~Enemy() {
	for (EnemyBullet* bullet : bullets_) {
		delete bullet;

	}

	for (TimedCall* timedCall : timedCalls_) {
		delete timedCall;
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
	worldTransform_.translation_ = { 5.0f, 0.0f, 50.0f };
	
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	// 最初の状態
	state_ = new EnemyStateApproach();
	state_->SetEnemy(this);

	ApproachPhaseInitialize();

}

// 接近フェーズ初期化
void Enemy::ApproachPhaseInitialize() {
	// 最初の発射を予約
	FireAndReset();

}

// 発射してリセット
void Enemy::FireAndReset() {

	// 弾を発射
	Fire();

	timedCalls_.push_back(
		new TimedCall(std::bind(&Enemy::FireAndReset, this), kFireInterval)
	);

}


/*-------------------------------
攻撃
--------------------------------*/
void Enemy::Fire() {

	assert(player_);

	// 弾の速さ
	const float kBulletSpeed = 1.0f;

	Vector3 playerPos = player_->GetworldPosition();
	// 敵キャラ自身のワールド座標を取得する
	Vector3 enemyPos = GetWorldPosition();

	// 敵から自キャラへの差分ベクトルを求める
	Vector3 velocity;
	velocity.x = playerPos.x - enemyPos.x;
	velocity.y = playerPos.y - enemyPos.y;
	velocity.z = playerPos.z - enemyPos.z;

	// ベクトルの正規化

	float length = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
	if (length != 0.0f) {
		velocity.x /= length;
		velocity.y /= length;
		velocity.z /= length;
	}

	// ベクトルの長さを速さに合わせる
	velocity.x *= kBulletSpeed;
	velocity.y *= kBulletSpeed;
	velocity.z *= kBulletSpeed;

	EnemyBullet* newBullet = new EnemyBullet();
	newBullet->SetPlayer(player_);
	newBullet->Initialize(model_, enemyPos, velocity);

	// 弾を登録する
	bullets_.push_back(newBullet);




}

/*----------------------------------------
衝突時コールバック
-----------------------------------*/
void Enemy::OnCollision() {

}


/*--------------------------
更新処理
------------------------------------*/
void Enemy::Update() {

	// 終了したイベントを削除
	timedCalls_.remove_if([](TimedCall* timedCall) {
		if (timedCall->isFinished()) {
			delete timedCall;
			return true;
		}
		return false;
		});


	for (TimedCall* timedCall : timedCalls_) {
		timedCall->Update();
	}


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

// 時限発動イベントのクリア
void Enemy::ClearTimedCalls() {
	for (TimedCall* timedCall : timedCalls_) {
		delete timedCall;
	}
	timedCalls_.clear();
}

Vector3 Enemy::GetWorldPosition() {
	// ワールド座標を入れる変数
	Vector3 worldPos;
	// ワールド行列の平行移動成分を取得
	worldPos.x = worldTransform_.matWorld_.m[3][0]; 
	worldPos.y = worldTransform_.matWorld_.m[3][1]; 
	worldPos.z = worldTransform_.matWorld_.m[3][2]; 

	return worldPos;
}