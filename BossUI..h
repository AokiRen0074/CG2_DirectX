#pragma once
#include "NeonModel.h"
#include "ViewProjection.h"
#include "WorldTransform.h"

class BossUI {
public:
	~BossUI();
	void Initialize();
	void Update(int currentHp, int maxHp, bool isBattleStarted);
	void Draw();
	void DrawImGui(); 

private:
	NeonModel* barModel_ = nullptr;
	NeonModel* frameModel_ = nullptr;
	uint32_t whiteTex_ = 0;

	WorldTransform barTransform_;
	WorldTransform frameTransform_;
	ViewProjection uiViewProjection_;

	float displayRatio_ = 0.0f;
	float appearTimer_ = 0.0f;
	bool isStarted_ = false;


	float barMaxWidth_ = 35.0f;
	float barHeight_ = 0.8f;
	float barPosX_ = 0.0f;
	float barPosY_ = 14.0f;

	float frameColor_[3] = { 1.0f, 0.2f, 0.0f };
	float frameIntensity_ = 2.0f;

	float barColor_[3] = { 1.0f, 0.2f, 0.2f };
	float barIntensity_ = 15.0f;
};