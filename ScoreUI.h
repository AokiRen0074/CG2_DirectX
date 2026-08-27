#pragma once
#include "DirectXCommon.h"
#include "ViewProjection.h"
#include "WorldTransform.h"
#include "NeonModel.h"
#include <string>

class ScoreUI {
public:
	~ScoreUI();
	void Initialize(DirectXCommon* dxCommon, uint32_t whiteTex);
	void Update();
	void Draw();
	void DrawImGui();

	void AddScore(int score) {
		targetScore_ += score;
		popupScore_ += score;
		popupTimer_ = popupMaxTime_;
	}

	void SetScore(int score) {
		targetScore_ = score;
		currentDisplayScore_ = (float)score;
		popupScore_ = 0; // ポップアップも消す
		popupTimer_ = 0.0f;
	}

	void Reset() { targetScore_ = 0; currentDisplayScore_ = 0; popupScore_ = 0; popupTimer_ = 0.0f; }

private:
	DirectXCommon* dxCommon_ = nullptr;
	uint32_t whiteTex_ = 0;


	NeonModel* numberModels_[10];       // メインスコア用
	NeonModel* popupNumberModels_[10];  // ポップアップ専用

	// ＋マーク用
	NeonModel* plusModel_ = nullptr;
	WorldTransform plusTransform_;

	ViewProjection uiViewProjection_;

	static const int kMaxDigits = 6;
	WorldTransform digitTransforms_[kMaxDigits];
	WorldTransform popupTransforms_[kMaxDigits];

	int targetScore_ = 0;
	float currentDisplayScore_ = 0;
	std::string currentDisplayString_ = "000000";

	int popupScore_ = 0;
	float popupTimer_ = 0.0f;
	float popupMaxTime_ = 90.0f;

	float textColor_[3] = { 1.0f, 1.0f, 1.0f };
	float textIntensity_ = 6.0f;
	float textOffsetX_ = -5.0f;
	float textOffsetY_ = -15.0f;
	float textScale_ = 1.0f;
	float textSpacing_ = 2.0f;
	float textRotY_ = 3.14159f;

	float popupColor_[3] = { 1.0f, 0.8f, 0.0f };
	float popupIntensity_ = 8.0f;
	float popupOffsetY_ = 3.0f;
	float popupScale_ = 0.7f;
};