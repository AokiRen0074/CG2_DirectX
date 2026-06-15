#include "AxisIndicator.h"
#include"TextureManager.h"

AxisIndicator* AxisIndicator::GetInstance() {
    static AxisIndicator instance;
    return &instance;
}

AxisIndicator::~AxisIndicator() {
    delete object3d_;
}

void AxisIndicator::Initialize() {
    object3d_ = new Object3d();
    object3d_->Initialize("Resources", "axis.obj");
   textureHandle_ = TextureManager::Load("Resources/axis.png");

    worldTransform_.Initialize();
    worldTransform_.scale_ = { 1.0f, 1.0f, 1.0f }; 
    worldTransform_.rotation_ = { 0.0f, 0.0f, 0.0f };

    worldTransform_.translation_ = { 6.0f, 4.0f, 0.0f };

    axisViewProjection_.Initialize();
    axisViewProjection_.translation_ = { 0.0f, 0.0f, -15.0f };
    axisViewProjection_.UpdateMatrix();
}

void AxisIndicator::Update() {

    if (!isVisible_ || !targetCamera_) { return; }

    worldTransform_.rotation_ = targetCamera_->rotation_;

    // 行列の更新と転送
    worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
    worldTransform_.TransferMatrix();
}

void AxisIndicator::Draw() {
    // 非表示なら描画しない
    if (!isVisible_ || !object3d_) { return; }

    object3d_->Draw(worldTransform_, axisViewProjection_,textureHandle_);
}