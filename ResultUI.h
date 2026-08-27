#pragma once
#include "NeonModel.h"
#include "WorldTransform.h"
#include "ViewProjection.h"
#include <string>
#include "NeonText.h"

struct ResultUIState {
	bool isActive = false;
	bool isFinished = false;
	float timer = 0.0f;
};

class ResultUI {
public:
	void Initialize(const std::string& directoryPath, DirectXCommon* dxCommon);
	void Start(int score, float clearTime);
	void Update(float deltaTime);
	void Draw(const ViewProjection& viewProjection);
	void DrawImGui();

	bool IsActive() const { return state_.isActive; }
	bool IsFinished() const { return state_.isFinished; }
	void Stop() { state_.isActive = false; }

private:
	void RebuildText();

	ResultUIState state_;
	DirectXCommon* dxCommon_ = nullptr;

	NeonModel* mainTextModel_ = nullptr;
	NeonModel* promptModel_ = nullptr;

	// 🌟 おしゃれな枠（WarningUIからの流用）
	NeonModel* frameModel_ = nullptr;
	WorldTransform transformFrameTop_;
	WorldTransform transformFrameBottom_;

	uint32_t whiteTex_ = 0;

	int finalScore_ = 0;
	float finalTime_ = 0.0f;

	NeonText* neonScore_ = nullptr;
	NeonText* neonTime_ = nullptr;

	WorldTransform transformMain_;
	WorldTransform transformPrompt_;
	float promptBlink_ = 0.0f;

	ViewProjection uiViewProjection_;

	// ==========================================
	// 🌟 ImGui / JSON パラメータ
	// ==========================================
	float mainTextRotY_ = 0.0f; // 🌟 これで裏返しを一発で直せます！
	float mainTextColor_[3] = { 0.0f, 1.0f, 1.0f };
	float mainTextIntensity_ = 5.0f;
	float mainTextOffsetY_ = 5.0f;
	float mainTextScale_ = 1.0f;

	float scoreTextColor_[3] = { 0.8f, 0.9f, 1.0f };
	float scoreTextIntensity_ = 3.0f;
	float scoreTextOffsetX_ = -5.0f;
	float scoreTextOffsetY_ = 1.0f;
	float scoreTextScale_ = 1.0f;
	float timeTextOffsetY_ = -3.0f;

	float textRadius_ = 0.01f;   // 🌟 四角く潰れないように初期値を「極細」に設定！
	float textSoftness_ = 5.0f;
	float textLengthOffset_ = 0.0f;

	float promptColor_[3] = { 1.0f, 1.0f, 1.0f };
	float promptIntensity_ = 3.0f;
	float promptOffsetY_ = -6.0f;
	float promptScale_ = 0.6f;

	float frameColor_[3] = { 0.0f, 0.8f, 1.0f }; // サイバーな青枠
	float frameIntensity_ = 3.0f;
	float frameOffsetY_ = 9.0f;

	float uiBaseZ_ = 0.0f;
	float animMainInTime_ = 1.0f; // 🌟 不足していたアニメーション時間変数
};