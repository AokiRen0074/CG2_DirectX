#pragma once
#include <cmath>
#include <cstdlib>

class FlickerTimer {
public:
	float time_ = 0.0f;
	bool isFlicker_ = true;

	// ベースの輝度を受け取り、チカチカ適用後の輝度を返す
	float GetIntensity(float baseIntensity) {
		if (!isFlicker_) return baseIntensity;

		time_ += 1.0f / 60.0f;
		float currentIntensity = baseIntensity;

		// sin波で明滅のリズムを作る
		if (std::sinf(time_ * 12.0f) > 0.7f) {
			float noise = (rand() % 100) / 100.0f;
			currentIntensity *= (0.2f + noise * 0.8f);
		}

		// 1%の確率で消灯
		if (rand() % 1000 < 10) {
			currentIntensity = 0.0f;
		}
		return currentIntensity;
	}
};