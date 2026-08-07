#include "RailCamera.h"
#include <cmath>

void RailCamera::Initialize(Rail* rail) {
	rail_ = rail;
	t_ = 0.0f;
	speed_ = 0.001f; // カメラが進むスピード

	viewProjection_.Initialize();
}

void RailCamera::Update() {
    if (!rail_) return;

    if (isPlay_) {
        t_ += speed_;
        if (t_ > 1.0f) {
            t_ = 1.0f;
            isPlay_ = false; // 終点で自動停止
        }
    }

    Vector3 eye = rail_->GetPosition(t_);
    Vector3 forward = rail_->GetForward(t_);

    rotate_.y = std::atan2(forward.x, forward.z);
    float xzLength = std::sqrt(forward.x * forward.x + forward.z * forward.z);
    rotate_.x = std::atan2(-forward.y, xzLength);
    rotate_.z = 0.0f;

    translate_ = eye;

    worldMatrix_ = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, rotate_, translate_);
    viewProjection_.translation_ = translate_;
    viewProjection_.rotation_ = rotate_;
    viewProjection_.UpdateMatrix();
}