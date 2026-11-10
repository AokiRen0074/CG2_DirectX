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

    // 現在の座標と描画カメラから、GPUへ渡す行列を更新する。
    worldTransform_.UpdateMatrix(viewProjection);
    model_->Draw(worldTransform_, viewProjection, textureHandle_);
}
