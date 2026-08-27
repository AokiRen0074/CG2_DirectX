#pragma once
#include "Vector3.h"

class CameraShake {
public:
	void Initialize();
	void Update();
	void Start(float intensity, int durationFrames);

	// 揺れのズレを取得
	Vector3 GetOffset() const { return currentOffset_; }

private:
	int shakeTimer_ = 0;
	float intensity_ = 0.0f;
	Vector3 currentOffset_ = { 0.0f, 0.0f, 0.0f };
};