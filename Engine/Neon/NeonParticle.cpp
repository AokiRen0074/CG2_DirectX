#include "NeonParticle.h"
#include <random>
#include <cmath>

void NeonParticle::Initialize(NeonModel* model, const Vector3& position, const Vector3& direction, float speed, float thickness, const Vector3& color) {
	model_ = model;
	worldTransform_.Initialize();

	// 🌟 修正：中心で一つの塊にならないよう、発生位置を数ミリだけランダムに散らす
	worldTransform_.translation_.x = position.x + ((float)(rand() % 100) / 100.0f - 0.5f) * 0.5f;
	worldTransform_.translation_.y = position.y + ((float)(rand() % 100) / 100.0f - 0.5f) * 0.5f;
	worldTransform_.translation_.z = position.z;

	velocity_ = direction;
	speed_ = speed;
	thickness_ = thickness;
	color_ = color;

	// 15〜30フレーム（一瞬）で素早く消えるようにする
	maxLife_ = 15 + (rand() % 15);
	life_ = maxLife_;

	// 🌟 修正：画面(XY平面)に対して綺麗に放射状に飛ぶよう、Z軸のみを回転させる
	worldTransform_.rotation_ = { 0.0f, 0.0f, std::atan2(velocity_.y, velocity_.x) };
}

void NeonParticle::Update() {
	// 座標を移動
	worldTransform_.translation_.x += velocity_.x * speed_;
	worldTransform_.translation_.y += velocity_.y * speed_;
	worldTransform_.translation_.z += velocity_.z * speed_;

	// 🌟 強烈な摩擦（急ブレーキ）。ドカンと広がってピタッと止まる Wavecade のキモ。
	speed_ *= 0.8f;

	// 寿命の割合 (1.0 -> 0.0)
	float t = (float)life_ / (float)maxLife_;

	// 太さは寿命とともにスッと消える
	float currentThickness = thickness_ * t;

	// 🌟 X軸(長さ)を「現在のスピード」に比例させる。速い時ほど長く、遅くなると点になる（疑似モーションブラー）
	float currentLength = speed_ * 1.2f + currentThickness;
	worldTransform_.scale_ = { currentLength, currentThickness, currentThickness };

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	life_--;
}

void NeonParticle::Draw(const ViewProjection& viewProjection, uint32_t textureHandle) {
	if (model_) {
		float t = (float)life_ / (float)maxLife_;

		// 最初は明るく、後からフワッと暗くなる
		float intensity = 20.0f * t * t;

		Vector3 drawColor = color_;

		// 🌟 オーバーブライト：発生直後（t > 0.7）は色が「白飛び」する
		if (t > 0.7f) {
			float mix = (t - 0.7f) / 0.3f; // 0.0 -> 1.0
			drawColor.x = color_.x + (1.0f - color_.x) * mix;
			drawColor.y = color_.y + (1.0f - color_.y) * mix;
			drawColor.z = color_.z + (1.0f - color_.z) * mix;
		}

		if (intensity > 0.1f) {
			model_->SetNeonColor(intensity, drawColor.x, drawColor.y, drawColor.z);
			model_->Draw(worldTransform_, viewProjection, textureHandle);
		}
	}
}