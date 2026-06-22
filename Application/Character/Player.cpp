#include "Application/Character/Player.h"
#include <cassert>
#include "DirectXCommon.h"
#include <algorithm>
#include <externals/nlohmann/json.hpp>
#include "GlobalValiables.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

/*----------------
初期化
-----------------------*/
void Player::Initialize(Object3d* model, uint32_t textureHandle) {
	assert(model);

	model_ = model;

	textureHandle_ = textureHandle;

	worldTransform_.Initialize();


	worldTransform_.scale_ = { 1.0f, 1.0f, 1.0f };
	worldTransform_.rotation_ = { 0.0f, 0.0f, 0.0f };
	worldTransform_.translation_ = { 0.0f, 0.0f, 0.0f };
	// シングルトンインスタンスを取得する
	input_ = Input::GetInstance();


	// デバッガによる確認
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "Player";

	// グループを追加
	GlobalVariables::GetInstance()->CreateGroup(groupName);

	globalVariables->AddItem(groupName, "Test", 90);

	globalVariables->AddItem(groupName, "moveSpeed", kCharacterSpeed);

}

/*--------------------
調整項目の適用
-------------------------*/
void Player::RegisterGlobalVariables() {}

void Player::ApplyGlobalVariables() {
	// 調整項目の適用
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "Player";

	kCharacterSpeed= globalVariables->GetFloatValue(groupName, "moveSpeed");
}

/*----------------------
旋回処理
------------------------------*/
void Player::Rotate() {

// 回転の速さ
	const float kRotaSpeed = 0.02f;

	// 押した方向で移動ベクトルを変更
	if (input_->PushKey(DIK_A)) {
		worldTransform_.rotation_.y -= kRotaSpeed;
	}
	else if (input_->PushKey(DIK_D)) {
		worldTransform_.rotation_.y += kRotaSpeed;
	}

}

/*------------------------
攻撃
----------------------------*/
void Player::Attack() {

	if (input_->TriggerKey(DIK_SPACE)) {

		// 弾を生成し初期イカ
		PlayerBullet* newBullet = new PlayerBullet();
		newBullet->Initialize(bulletModel_, worldTransform_.translation_);

		// 弾を登録する
		bullet_ = newBullet;
	}
}

/*-------------------------
更新処理
----------------------------*/
void Player::Update() {

	// 機能の調整
	ApplyGlobalVariables();

	//　旋回処理
	Rotate();

#ifdef USE_IMGUI

	// キャラクターの座標を画面表示する処理

	ImGui::Begin("Player");

	ImGui::Text("Position: X: %f, Y: %f, Z: %f",
		worldTransform_.translation_.x,
		worldTransform_.translation_.y,
		worldTransform_.translation_.z);

	ImGui::End();

#endif

	// キャラクターの移動ベクトル
	Vector3 move = { 0,0,0 };


	// 押した方向へ移動ベクトルを変更(左右)
	if (input_->PushKey(DIK_LEFT)) {
		move.x -= kCharacterSpeed;
	}
	else if (input_->PushKey(DIK_RIGHT)) {
		move.x += kCharacterSpeed;
	}

	// 押した方向へ移動ベクトル(上下)
	if (input_->PushKey(DIK_UP)) {
		move.y += kCharacterSpeed;
	}
	else if (input_->PushKey(DIK_DOWN)) {
		move.y -= kCharacterSpeed;
	}


	// 座標移動
	worldTransform_.translation_.x += move.x;
	worldTransform_.translation_.y += move.y;
	worldTransform_.translation_.z += move.z;

	// 移動限界座標
	const float kMoveLimitX = 5.5f;
	const float kMoveLimitY = 2.7f;

	// 範囲を超えない処理
	worldTransform_.translation_.x = (std::max)(worldTransform_.translation_.x, -kMoveLimitX);
	worldTransform_.translation_.x = (std::min)(worldTransform_.translation_.x, kMoveLimitX);
	worldTransform_.translation_.y = (std::max)(worldTransform_.translation_.y, -kMoveLimitY);
	worldTransform_.translation_.y = (std::min)(worldTransform_.translation_.y, kMoveLimitY);

	// 攻撃処理
	Attack();

	// 弾更新
	if (bullet_) {
		bullet_->Update();
	}

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	worldTransform_.TransferMatrix();
}



/*--------------------------
描画処理
--------------------*/
void Player::Draw(const ViewProjection& viewProjection) {

	model_->Draw(worldTransform_, viewProjection, textureHandle_);

	// 弾描画
	if (bullet_) {
		bullet_->Draw(viewProjection);
	}

}