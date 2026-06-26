#include "Enemy.h"
#include <cassert>


#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

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

/*-------------------------------
フェーズの管理
-------------------------------*/
// 接近フェーズ
void Enemy::ApproachPhase() {
	// 移動
	worldTransform_.translation_.z += approachVelocity_.z;

	if (worldTransform_.translation_.z < 0.0f) {
		phase_ = Phase::Leave;
	}
}

// 離脱フェーズ
void Enemy::LeavePhase() {
	// 移動
	worldTransform_.translation_.z += leaveVelocity_.z;
}

/*--------------------------
更新処理
------------------------------------*/
void Enemy::Update() {

#ifdef USE_IMGUI

	// キャラクターの座標を画面表示する処理

	ImGui::Begin("Enemy");

	ImGui::Text("Position: X: %f, Y: %f, Z: %f",
		worldTransform_.translation_.x,
		worldTransform_.translation_.y,
		worldTransform_.translation_.z);

	ImGui::End();

#endif


	// 状態遷移
	switch (phase_) {
	case Phase::Approach:
	default:
		ApproachPhase();
		break;

	case Phase::Leave:
		LeavePhase();
		break;

	}

	// 行列の更新
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	worldTransform_.TransferMatrix();
}


// 描画処理
void Enemy::Draw(const ViewProjection& viewProjection) {

	// 敵の描画
	model_->Draw(worldTransform_, viewProjection, textureHandle_);


}