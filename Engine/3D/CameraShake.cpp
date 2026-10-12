#include "CameraShake.h"
#include <cstdlib> 

void CameraShake::Initialize() {
	shakeTimer_ = 0;
	intensity_ = 0.0f;
	currentOffset_ = { 0.0f, 0.0f, 0.0f };
}

void CameraShake::Update() {
	if (shakeTimer_ > 0) {
		shakeTimer_--;
		// ランダムな方向に激しくブレる
		currentOffset_ = {
			(rand() % 100 - 50) / 100.0f * intensity_,
			(rand() % 100 - 50) / 100.0f * intensity_,
			(rand() % 100 - 50) / 100.0f * intensity_
		};
		// 揺れをだんだん収束させる
		intensity_ *= 0.85f;
	}
	else {
		currentOffset_ = { 0.0f, 0.0f, 0.0f };
	}
}

void CameraShake::Start(float intensity, int durationFrames) {
	intensity_ = intensity;
	shakeTimer_ = durationFrames;
}