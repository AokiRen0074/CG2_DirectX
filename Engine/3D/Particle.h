#pragma once
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"
#include <cmath>
#include <stdexcept>

// 発生座標・移動速度・表示サイズ・寿命を、粒ごとに指定する
struct ParticleParameters {
	Vector3 position = { 0.0f, 0.0f, 0.0f };
	Vector3 velocity = { 0.0f, 0.0f, 0.0f };
	Vector2 size = { 1.0f, 1.0f };
	Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
	float lifetime = 1.0f;
};

// 描画リソースはグループで共有し、粒自身は座標と寿命を持つ
class Particle {
public:
	explicit Particle(const ParticleParameters& parameters)
		: parameters_(parameters), remainingLife_(parameters.lifetime) {
		if (!std::isfinite(parameters.position.x) || !std::isfinite(parameters.position.y) ||
			!std::isfinite(parameters.position.z) || !std::isfinite(parameters.velocity.x) ||
			!std::isfinite(parameters.velocity.y) || !std::isfinite(parameters.velocity.z) ||
			!std::isfinite(parameters.size.x) || !std::isfinite(parameters.size.y) ||
			!std::isfinite(parameters.color.x) || !std::isfinite(parameters.color.y) ||
			!std::isfinite(parameters.color.z) || !std::isfinite(parameters.color.w) ||
			!std::isfinite(parameters.lifetime) || parameters.lifetime <= 0.0f ||
			parameters.size.x <= 0.0f || parameters.size.y <= 0.0f) {
			throw std::invalid_argument("Invalid particle parameters");
		}
	}

	void Update(float deltaTime) {
		if (!std::isfinite(deltaTime) || deltaTime < 0.0f) {
			throw std::invalid_argument("Invalid particle update interval");
		}

		parameters_.position.x += parameters_.velocity.x * deltaTime;
		parameters_.position.y += parameters_.velocity.y * deltaTime;
		parameters_.position.z += parameters_.velocity.z * deltaTime;
		remainingLife_ -= deltaTime;
	}

	bool IsAlive() const {
		return remainingLife_ > 0.0f;
	}

	const Vector3& GetPosition() const {
		return parameters_.position;
	}

	const Vector2& GetSize() const {
		return parameters_.size;
	}

	Vector4 GetColor() const {
		Vector4 color = parameters_.color;
		const float remaining = IsAlive() ? remainingLife_ / parameters_.lifetime : 0.0f;
		color.w *= remaining;
		return color;
	}

private:
	ParticleParameters parameters_;
	float remainingLife_ = 0.0f;
};