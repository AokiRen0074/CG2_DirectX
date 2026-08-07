#include "AxisIndicator.h"
#include "TextureManager.h"

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

    object3d_->GetTransform().scale = { 0.2f, 0.2f, 0.2f };
    object3d_->GetTransform().rotate = { 0.0f, 0.0f, 0.0f };
    object3d_->GetTransform().translate = { 0.0f, 0.0f, 0.0f };
}

void AxisIndicator::Update() {
    if (!isVisible_ || !targetCamera_) { return; }

    // ターゲットカメラから行列をコピー
    Matrix4x4 matView = targetCamera_->matView;
    Matrix4x4 matProjection = targetCamera_->matProjection;

    // 画面右上に固定するためのハック
    matView.m[3][0] = 5.0f;
    matView.m[3][1] = 3.0f;
    matView.m[3][2] = 15.0f;


    object3d_->SetCameraMatrix(matView, matProjection);
    object3d_->Update();
}

void AxisIndicator::Draw() {
    // 非表示なら描画しない
    if (!isVisible_ || !object3d_) { return; }

    WorldTransform dummyWT;
    ViewProjection dummyVP;
    object3d_->Draw(dummyWT, dummyVP, textureHandle_);
}