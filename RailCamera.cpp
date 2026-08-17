#include "RailCamera.h"
#include <cmath>

void RailCamera::Initialize(Rail* rail) {
	rail_ = rail;
	t_ = 0.0f;
	speed_ = 0.001f; // カメラが進むスピード

	viewProjection_.Initialize();
    viewProjection_.matProjection = MakePerspectiveFovMatrix(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);
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
    Vector3 localOffset = { 0.0f, 0.0f, -50.0f };

    Matrix4x4 rotMatrix = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, rotate_, { 0,0,0 });
    Vector3 worldOffset = TransformNormal(localOffset, rotMatrix);


    viewProjection_.translation_.x = translate_.x + worldOffset.x;
    viewProjection_.translation_.y = translate_.y + worldOffset.y;
    viewProjection_.translation_.z = translate_.z + worldOffset.z;


    viewProjection_.rotation_ = rotate_;

    viewProjection_.UpdateMatrix();
}