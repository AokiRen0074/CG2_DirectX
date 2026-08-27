#pragma once
#include "NeonModel.h"
#include "WorldTransform.h"
#include "ViewProjection.h"
#include <string>

class WarningUI {
private:
	WarningUI() = default;
	~WarningUI();
	WarningUI(const WarningUI&) = delete;
	WarningUI& operator=(const WarningUI&) = delete;

public:
	static WarningUI* GetInstance() {
		static WarningUI instance;
		return &instance;
	}

	void Initialize(const std::string& directoryPath);
	void Update();
	void Draw();
	void DrawImGui();
	void StartWarning();
	bool IsFinished() const { return isFinished_; }

private:
	NeonModel* modelFrame_ = nullptr;
	NeonModel* modelText_ = nullptr;
	NeonModel* modelIcon_ = nullptr;

	WorldTransform transformFrameTop_;
	WorldTransform transformFrameBottom_;
	WorldTransform transformText_;
	WorldTransform transformIconL_;
	WorldTransform transformIconR_;

	ViewProjection uiViewProjection_;
	uint32_t whiteTexture_ = 0;

	bool isActive_ = false;
	bool isFinished_ = false;
	float timer_ = 0.0f;
	float animeTime_ = 0.0f;

	// 🌟 追加：調整のためにUIを出しっぱなしにするデバッグフラグ
	bool isDebugKeepActive_ = false;

	// ==========================================
	// 🌟 ImGui ＆ JSON保存用のレイアウト変数群
	// ==========================================
	float textColor_[3] = { 1.0f, 0.0f, 0.0f };
	float textBaseIntensity_ = 2.0f;
	float textFlashIntensity_ = 15.0f;

	float frameColor_[3] = { 0.8f, 0.8f, 0.8f };
	float frameIntensity_ = 2.0f;
	float frameFlashColor_[3] = { 1.0f, 1.0f, 1.0f };
	float frameFlashIntensity_ = 30.0f;

	float frameScaleX_ = 1.0f;
	float frameScaleY_ = 1.0f;
	float frameOffsetY_ = 7.0f;

	float textOffsetX_ = 0.0f;
	float textOffsetY_ = 0.0f;
	float textScale_ = 1.0f;

	float iconOffsetX_ = 12.0f;
	float iconOffsetY_ = 0.0f;
	float iconScale_ = 0.8f;
};