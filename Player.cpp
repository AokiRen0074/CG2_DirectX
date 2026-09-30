
#include "Application/Character/Player/Player.h"

/*-------------------------
初期化
-----------------------*/
void Player::Initialize() {
	model3D_ = new Object3d();
	model3D_->Initialize("Resources", "cube.obj");
    model3D_->GetTransform().translate = { -5.0f, 0.0f, 0.0f };
    model3D_->GetTransform().scale = { 0.5f, 0.5f, 0.5f };
}

/*--------------------
更新処理
------------------------*/
void Player::Update() {
    const float kGravity = -0.05f;
    const float kJumpVelocity = 0.8f;

    if (Input::GetInstance()->TriggerKey(DIK_SPACE) && !isJumping_) {
        velocityY_ = kJumpVelocity;
        isJumping_ = true;
    }

    velocityY_ += kGravity;
    model3D_->GetTransform().translate.y += velocityY_;

    if (model3D_->GetTransform().translate.y <= 0.0f) {
        model3D_->GetTransform().translate.y = 0.0f;
        velocityY_ = 0.0f;
        isJumping_ = false;
    }

    model3D_->Update();
}

/*-----------------------
描画処理
-------------------------*/
void Player::Draw(const WorldTransform& worldTransform, const ViewProjection& viewProjection, uint32_t textureHandle) {
    model3D_->Draw(worldTransform, viewProjection, textureHandle);
}

void Player::OnCollision() {
    // 敵とぶつかった時の処理
}