#pragma once
#include "DirectXCommon.h"
#include "ViewProjection.h"
#include "WorldTransform.h"
#include "NeonModel.h"
#include <string>

class WaveUI {
public:
	~WaveUI();
	void Initialize(DirectXCommon* dxCommon, uint32_t whiteTex);
	void Update(int currentWave, bool isInterval);
	void Draw();
	void DrawImGui();

private:
	DirectXCommon* dxCommon_ = nullptr;
	uint32_t whiteTex_ = 0;

	// 数字とWAVE文字のモデル
	NeonModel* numberModels_[10];
	NeonModel* waveTextModel_ = nullptr; // 🌟 "WAVE"文字用モデル

	ViewProjection uiViewProjection_;

	static const int kMaxDigits = 3;
	WorldTransform digitTransforms_[kMaxDigits];
	WorldTransform waveTextTransform_; // 🌟 "WAVE"文字用トランスフォーム

	int displayWave_ = 1;
	bool wasInterval_ = false;

	enum class State {
		Center,
		Moving,
		BottomRight
	};
	State state_ = State::Center;

	float moveTimer_ = 0.0f;
	float moveMaxTime_ = 60.0f;

	float appearTimer_ = 0.0f;
	float appearMaxTime_ = 30.0f;

	float color_[3] = { 1.0f, 1.0f, 1.0f };
	float intensity_ = 6.0f;

	float centerX_ = 0.0f;
	float centerY_ = 15.0f;
	float centerScale_ = 3.0f;
	float shakeAmount_ = 0.5f;

	float bottomRightX_ = 15.0f;
	float bottomRightY_ = -22.0f;
	float bottomRightScale_ = 0.8f;

	float textSpacing_ = 2.0f;
	float rotY_ = 3.14159f;



	float swayTimer_ = 0.0f;

	float waveTextScaleRatio_ = 0.4f;
	float waveTextOffsetX_ = 0.0f;
	float waveTextOffsetY_ = -3.0f;

};