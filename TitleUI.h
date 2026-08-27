#pragma once
#include "NeonText.h"
#include "ViewProjection.h"
#include <string>

class TitleUI {
public:
	void Initialize(DirectXCommon* dxCommon);
	void Update(float deltaTime);
	void Draw();
	void DrawImGui();

	void SetActive(bool isActive) { isActive_ = isActive; }
	bool IsActive() const { return isActive_; }

	// 🌟 追加：スタート演出のトリガーとリセット
	void PlayStartAnimation() { isStarting_ = true; startTimer_ = 0.0f; }
	void Reset() { isStarting_ = false; startTimer_ = 0.0f; }

private:
	void RebuildText();

	char titleString_[64] = "NEON STRIKE";
	char promptString_[64] = "PUSH SPACE TO START";

	bool isActive_ = false;
	float timer_ = 0.0f;

	// 🌟 追加：アニメーション用
	bool isStarting_ = false;
	float startTimer_ = 0.0f;

	DirectXCommon* dxCommon_ = nullptr;
	NeonText* titleText_ = nullptr;
	NeonText* promptText_ = nullptr;
	ViewProjection uiViewProjection_;

	// ImGui parameters
	float titleColor_[3] = { 0.0f, 1.0f, 1.0f };
	float titleIntensity_ = 5.0f;
	float titleOffsetX_ = -3.5f;
	float titleOffsetY_ = 2.0f;
	float titleScale_ = 2.0f;

	float promptColor_[3] = { 1.0f, 1.0f, 1.0f };
	float promptIntensity_ = 3.0f;
	float promptOffsetX_ = -7.5f;
	float promptOffsetY_ = -3.0f;
	float promptScale_ = 0.8f;

	float textRadius_ = 0.01f;
	float textSoftness_ = 5.0f;
	float textLengthOffset_ = 0.0f;
};