#include "Skydome.h"
#include <cassert>

void Skydome::Initialize(Object3d* model,uint32_t textureHandle) {

	assert(model);
	model_ = model;
	textureHandle_ = textureHandle;

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();
	worldTransform_.scale_ = { 1.0f, 1.0f, 1.0f }; 
	worldTransform_.rotation_ = { 0.0f, 0.0f, 0.0f };
	worldTransform_.translation_ = { 0.0f, 0.0f, 0.0f };
}

void Skydome::Update() {
	// 行列の更新
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	// 定数バッファに転送
	worldTransform_.TransferMatrix();
}

void Skydome::Draw(const ViewProjection& viewProjection) {
	// 描画

	model_->Draw(worldTransform_, viewProjection,textureHandle_);
}