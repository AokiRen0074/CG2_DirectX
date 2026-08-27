#pragma once
#include "DirectXCommon.h"
#include "ViewProjection.h"
#include "WorldTransform.h"
#include "NeonModel.h"

class RebootUI {
public:
	~RebootUI();
	void Initialize(DirectXCommon* dxCommon, uint32_t whiteTex);
	void Update();
	void Draw();
	void DrawImGui();


	void Start();
	bool IsFinished() const { return isFinished_; }
	bool IsActive() const { return isActive_; }

private:
	DirectXCommon* dxCommon_ = nullptr;
	uint32_t whiteTex_ = 0;

	NeonModel* rebootModel_ = nullptr;
	ViewProjection uiViewProjection_;
	WorldTransform transform_;

	bool isActive_ = false;
	bool isFinished_ = false;
	float timer_ = 0.0f;
	float maxTime_ = 120.0f; // 約2秒間で再起動

	// ImGuiパラメータ（警告っぽい赤色）
	float color_[3] = { 1.0f, 0.0f, 0.2f };
	float baseIntensity_ = 8.0f;
	float currentIntensity_ = 0.0f;
	float scale_ = 2.0f;
	float offsetY_ = 5.0f;
};