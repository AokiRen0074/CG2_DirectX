
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


    worldTransform_.Initialize();
    worldTransform_.scale_ = { 0.2f, 0.2f, 0.2f }; 
    worldTransform_.rotation_ = { 0.0f, 0.0f, 0.0f };

    worldTransform_.translation_ = { 5.0f, 3.5f, 0.0f };

    axisViewProjection_.Initialize();
    axisViewProjection_.translation_ = { 0.0f, 0.0f, -15.0f };
    axisViewProjection_.UpdateMatrix();
}

void AxisIndicator::Update() {



    if (!isVisible_ || !targetCamera_) { return; }


    axisViewProjection_.matProjection = targetCamera_->matProjection;

    axisViewProjection_.matView = targetCamera_->matView;

    axisViewProjection_.matView.m[3][0] = 5.0f; 
    axisViewProjection_.matView.m[3][1] = 3.0f;
    axisViewProjection_.matView.m[3][2] = 15.0f;

    worldTransform_.translation_ = { 0.0f, 0.0f, 0.0f };
    worldTransform_.rotation_ = { 0.0f, 0.0f, 0.0f };
    worldTransform_.UpdateMatrix(axisViewProjection_);
}

void AxisIndicator::Draw() {
    // 非表示なら描画しない
    if (!isVisible_ || !object3d_) { return; }
    object3d_->Draw(worldTransform_, axisViewProjection_, textureHandle_);
}
