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

	// 進行度を進める
	t_ += speed_;
	if (t_ > 1.0f) {
		t_ = 1.0f; // 終点でストップさせる
	}

	// レールから座標と前方ベクトルを取得
	Vector3 eye = rail_->GetPosition(t_);
	Vector3 forward = rail_->GetForward(t_);

	// 前方ベクトルからオイラー角を計算
	rotate_.y = std::atan2(forward.x, forward.z);
	float xzLength = std::sqrt(forward.x * forward.x + forward.z * forward.z);
	rotate_.x = std::atan2(-forward.y, xzLength);
	rotate_.z = 0.0f; // ロール回転は基本0

	translate_ = eye;

	// ViewProjection と WorldMatrix に反映
	worldMatrix_ = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, rotate_, translate_);

	viewProjection_.translation_ = translate_;
	viewProjection_.rotation_ = rotate_;
	viewProjection_.UpdateMatrix();
}