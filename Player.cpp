#include "Player.h"
#include <cassert>
#include "DirectXCommon.h"


/*----------------
初期化
-----------------------*/
void Player::Initialize(Object3d* model, uint32_t textureHandle) {
	assert(model);

	model_ = model;

	textureHandle_ = textureHandle;

	worldTransform_.Initialize();


}

/*-------------------------
更新処理
----------------------------*/
void Player::Update() {
	worldTransform_.TransferMatrix();
}


/*--------------------------
描画処理
--------------------*/
void Player::Draw(const ViewProjection& viewProjection) {

	model_->Draw(worldTransform_, viewProjection, textureHandle_);

}