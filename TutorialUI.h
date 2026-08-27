#pragma once
#include "DirectXCommon.h"
#include "ViewProjection.h"
#include "WorldTransform.h"
#include "NeonModel.h"

class TutorialUI {
public:
	~TutorialUI();
	void Initialize(DirectXCommon* dxCommon, uint32_t whiteTex, uint32_t blackTex);
	void Update(int currentWave, bool isInterval);
	void DrawBase();
	void DrawNeon();
	void DrawImGui();

private:
	DirectXCommon* dxCommon_ = nullptr;
	uint32_t whiteTex_ = 0;
	uint32_t blackTex_ = 0;

	// 🌟 作成していただいた4つのモデル
	NeonModel* moveBase_ = nullptr;
	NeonModel* moveNeon_ = nullptr;
	NeonModel* shootBase_ = nullptr;
	NeonModel* shootNeon_ = nullptr;

	ViewProjection uiViewProjection_;
	WorldTransform moveTransform_;
	WorldTransform shootTransform_;

	enum class State {
		Hidden,
		Appear,
		MoveToCorner,
		Active
	};
	State state_ = State::Hidden;

	float timer_ = 0.0f;
	float appearMaxTime_ = 30.0f;
	float moveMaxTime_ = 60.0f;
	float swayTimer_ = 0.0f;

	// アクション完了フラグと縮小用タイマー
	bool isMoveCompleted_ = false;
	bool isShootCompleted_ = false;
	float moveShrinkT_ = 0.0f;
	float shootShrinkT_ = 0.0f;

	// ImGuiパラメータ
	float color_[3] = { 0.0f, 0.8f, 1.0f }; // 爽やかなシアン
	float intensity_ = 8.0f;

	float centerX_ = 0.0f;
	float centerY_ = 5.0f;
	float centerScale_ = 2.0f;

	float cornerMoveX_ = -20.0f;
	float cornerMoveY_ = -18.0f;
	float cornerShootX_ = -10.0f;
	float cornerShootY_ = -18.0f;
	float cornerScale_ = 0.8f;

	float swayAmount_ = 0.3f;

	float rotX_ = -1.57f;
	float rotY_ = 0.0f;
	float rotZ_ = 0.0f;


};