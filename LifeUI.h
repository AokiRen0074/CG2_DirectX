#pragma once
#include "DirectXCommon.h"
#include "ViewProjection.h"
#include "WorldTransform.h"
#include "NeonModel.h"

class LifeUI {
public:
	~LifeUI();
	void Initialize(DirectXCommon* dxCommon, uint32_t whiteTex);
	void Update();
	void Draw();
	void DrawImGui();

	// 🌟 残機を減らす・セットする関数
	void DecreaseLife() { if (currentLife_ > 0) currentLife_--; }
	void SetLife(int life) { currentLife_ = life; }
	int GetLife() const { return currentLife_; }

private:
	DirectXCommon* dxCommon_ = nullptr;
	uint32_t whiteTex_ = 0;

	// 残機アイコンのモデル
	NeonModel* lifeModel_ = nullptr;

	ViewProjection uiViewProjection_;

	// 最大残機数分の座標データ
	static const int kMaxLives = 10;
	WorldTransform lifeTransforms_[kMaxLives];

	int currentLife_ = 5; // 初期残機

	// ImGuiパラメータ
	float iconColor_[3] = { 0.0f, 0.8f, 1.0f }; // シアン系のネオンカラー
	float iconIntensity_ = 6.0f;
	float iconOffsetX_ = 0.0f;
	float iconOffsetY_ = -22.0f; // スコアの下に配置する想定
	float iconScale_ = 0.8f;
	float iconSpacing_ = 2.0f;
	float iconRotY_ = 0.0f;
};