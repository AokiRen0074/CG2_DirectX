#include "Player.h"
#include <cassert>
#include "DirectXCommon.h"


/*----------------
初期化
-----------------------*/
void Player::Initialize(ModelData* model, uint32_t textureHandle) {
	assert(model);

	model_ = model;

	textureHandle_ = textureHandle;

	worldTransform_.Initialize(DirectXCommon:


}

/*-------------------------
更新処理
----------------------------*/
void Player::Update() {

}


/*--------------------------
描画処理
--------------------*/
void Player::Draw() {

}