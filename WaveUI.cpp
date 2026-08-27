#include "WaveUI.h"
#include "GlobalValiables.h"
#include <cmath>
#include <algorithm>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

namespace {
	float Lerp(float a, float b, float t) {
		return a + (b - a) * t;
	}
}

WaveUI::~WaveUI() {
	for (int i = 0; i < 10; ++i) {
		delete numberModels_[i];
	}
	delete waveTextModel_;
}

void WaveUI::Initialize(DirectXCommon* dxCommon, uint32_t whiteTex) {
	dxCommon_ = dxCommon;
	whiteTex_ = whiteTex;

	for (int i = 0; i < 10; ++i) {
		numberModels_[i] = new NeonModel();
		std::string fileName = std::to_string(i) + ".obj";
		numberModels_[i]->Initialize("Resources/UI", fileName);
	}

	waveTextModel_ = new NeonModel();
	waveTextModel_->Initialize("Resources/UI", "wave_text.obj");

	for (int i = 0; i < kMaxDigits; ++i) {
		digitTransforms_[i].Initialize();
	}
	waveTextTransform_.Initialize();

	uiViewProjection_.Initialize();
	uiViewProjection_.translation_ = { 0.0f, 0.0f, -50.0f };
	uiViewProjection_.UpdateMatrix();

	GlobalVariables* global = GlobalVariables::GetInstance();
	const std::string groupName = "WaveUI";
	global->CreateGroup(groupName);
	global->AddItem(groupName, "Color", Vector3(1.0f, 1.0f, 1.0f));
	global->AddItem(groupName, "Intensity", 6.0f);
	global->AddItem(groupName, "CenterX", 0.0f);
	global->AddItem(groupName, "CenterY", 15.0f);
	global->AddItem(groupName, "CenterScale", 3.0f);
	global->AddItem(groupName, "ShakeAmount", 0.5f);
	global->AddItem(groupName, "BottomRightX", 15.0f);
	global->AddItem(groupName, "BottomRightY", -22.0f);
	global->AddItem(groupName, "BottomRightScale", 0.8f);
	global->AddItem(groupName, "TextSpacing", 2.0f);
	global->AddItem(groupName, "RotY", 3.14159f);

	global->AddItem(groupName, "WaveTextScaleRatio", 0.4f);
	global->AddItem(groupName, "WaveTextOffsetX", 0.0f); // 🌟 追加
	global->AddItem(groupName, "WaveTextOffsetY", -3.0f);

	Vector3 c = global->GetVector3Value(groupName, "Color");
	color_[0] = c.x; color_[1] = c.y; color_[2] = c.z;
	intensity_ = global->GetFloatValue(groupName, "Intensity");
	centerX_ = global->GetFloatValue(groupName, "CenterX");
	centerY_ = global->GetFloatValue(groupName, "CenterY");
	centerScale_ = global->GetFloatValue(groupName, "CenterScale");
	shakeAmount_ = global->GetFloatValue(groupName, "ShakeAmount");
	bottomRightX_ = global->GetFloatValue(groupName, "BottomRightX");
	bottomRightY_ = global->GetFloatValue(groupName, "BottomRightY");
	bottomRightScale_ = global->GetFloatValue(groupName, "BottomRightScale");
	textSpacing_ = global->GetFloatValue(groupName, "TextSpacing");
	rotY_ = global->GetFloatValue(groupName, "RotY");
	waveTextScaleRatio_ = global->GetFloatValue(groupName, "WaveTextScaleRatio");
	waveTextOffsetX_ = global->GetFloatValue(groupName, "WaveTextOffsetX"); // 🌟 追加
	waveTextOffsetY_ = global->GetFloatValue(groupName, "WaveTextOffsetY");
}

void WaveUI::Update(int currentWave, bool isInterval) {
	displayWave_ = currentWave;

	// 🌟 状態遷移の判定
	if (isInterval) {
		if (!wasInterval_) {
			// インターバル（ワープ）に「入った瞬間」に出現アニメーション開始！
			state_ = State::Center;
			appearTimer_ = 0.0f;
		}
	}
	else {
		if (wasInterval_) {
			// 戦闘開始の瞬間に移動アニメーション開始！
			state_ = State::Moving;
			moveTimer_ = 0.0f;
		}
	}
	wasInterval_ = isInterval;

	float targetX = 0.0f;
	float targetY = 0.0f;
	float targetScale = 1.0f;

	if (state_ == State::Center) {
		// ゆらゆら浮遊する処理
		swayTimer_ += 1.0f / 60.0f;
		float swayX = std::sin(swayTimer_ * 2.0f) * shakeAmount_;
		float swayY = std::cos(swayTimer_ * 1.5f) * shakeAmount_;

		targetX = centerX_ + swayX;
		targetY = centerY_ + swayY;

		// 🌟 出現イージング処理（EaseOutBack：少し通り過ぎて戻るスタイリッシュな動き）
		if (appearTimer_ < appearMaxTime_) {
			appearTimer_ += 1.0f;
			float t = appearTimer_ / appearMaxTime_;
			float tMinus1 = t - 1.0f;
			// 魔法の計算式：勢いよく飛び出して少し縮む
			float easeT = 1.0f + 2.70158f * std::pow(tMinus1, 3.0f) + 1.70158f * std::pow(tMinus1, 2.0f);

			// スケールを0から目標サイズまでアニメーション
			targetScale = centerScale_ * easeT;
		}
		else {
			targetScale = centerScale_; // アニメーション完了
		}
	}
	else if (state_ == State::Moving) {
		swayTimer_ = 0.0f;

		moveTimer_ += 1.0f;
		float t = moveTimer_ / moveMaxTime_;
		if (t >= 1.0f) {
			t = 1.0f;
			state_ = State::BottomRight;
		}

		float easeT = 1.0f - std::pow(1.0f - t, 3.0f);

		targetX = Lerp(centerX_, bottomRightX_, easeT);
		targetY = Lerp(centerY_, bottomRightY_, easeT);
		targetScale = Lerp(centerScale_, bottomRightScale_, easeT);
	}
	else if (state_ == State::BottomRight) {
		swayTimer_ = 0.0f;
		targetX = bottomRightX_;
		targetY = bottomRightY_;
		targetScale = bottomRightScale_;
	}

	std::string waveStr = std::to_string(displayWave_);
	float totalWidth = (static_cast<int>(waveStr.length()) - 1) * textSpacing_ * targetScale;
	float startX = targetX - totalWidth * 0.5f;

	for (size_t i = 0; i < waveStr.length() && i < kMaxDigits; ++i) {
		float drawX = startX + (i * textSpacing_ * targetScale);

		digitTransforms_[i].scale_ = { targetScale, targetScale, targetScale };
		digitTransforms_[i].rotation_ = { 0.0f, rotY_, 0.0f };
		digitTransforms_[i].translation_ = { drawX, targetY, 0.0f };
		digitTransforms_[i].matWorld_ = MakeAffineMatrix(digitTransforms_[i].scale_, digitTransforms_[i].rotation_, digitTransforms_[i].translation_);
		digitTransforms_[i].TransferMatrix();
	}

	float waveTextScale = targetScale * waveTextScaleRatio_;
	waveTextTransform_.scale_ = { waveTextScale, waveTextScale, waveTextScale };
	waveTextTransform_.rotation_ = { 0.0f, rotY_, 0.0f };
	waveTextTransform_.translation_ = {
		targetX + (waveTextOffsetX_ * targetScale),
		targetY + (waveTextOffsetY_ * targetScale),
		0.0f
	};
	waveTextTransform_.matWorld_ = MakeAffineMatrix(waveTextTransform_.scale_, waveTextTransform_.rotation_, waveTextTransform_.translation_);
	waveTextTransform_.TransferMatrix();
}

void WaveUI::Draw() {
	std::string waveStr = std::to_string(displayWave_);
	for (size_t i = 0; i < waveStr.length() && i < kMaxDigits; ++i) {
		char c = waveStr[i];
		int num = c - '0';
		if (num >= 0 && num <= 9) {
			NeonModel* model = numberModels_[num];
			model->SetNeonColor(intensity_, color_[0], color_[1], color_[2]);
			model->Draw(digitTransforms_[i], uiViewProjection_, whiteTex_);
		}
	}

	if (waveTextModel_) {
		waveTextModel_->SetNeonColor(intensity_, color_[0], color_[1], color_[2]);
		waveTextModel_->Draw(waveTextTransform_, uiViewProjection_, whiteTex_);
	}
}

void WaveUI::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("Wave UI Settings");

	ImGui::ColorEdit3("Color", color_);
	ImGui::SliderFloat("Intensity", &intensity_, 0.0f, 20.0f);

	if (ImGui::TreeNodeEx("Center Position (Interval)", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SliderFloat("Center X", &centerX_, -40.0f, 40.0f);
		ImGui::SliderFloat("Center Y", &centerY_, -40.0f, 40.0f);
		ImGui::SliderFloat("Center Scale", &centerScale_, 0.1f, 10.0f);
		ImGui::SliderFloat("Sway Amount", &shakeAmount_, 0.0f, 5.0f); // 🌟 ゆらゆらに名前変更
		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("Bottom Right Position (Playing)", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SliderFloat("Right X", &bottomRightX_, -40.0f, 40.0f);
		ImGui::SliderFloat("Right Y", &bottomRightY_, -40.0f, 40.0f);
		ImGui::SliderFloat("Right Scale", &bottomRightScale_, 0.1f, 5.0f);
		ImGui::TreePop();
	}

	ImGui::SliderFloat("Text Spacing", &textSpacing_, 0.1f, 5.0f);
	ImGui::SliderFloat("Rot Y", &rotY_, 0.0f, 6.28f);

	if (ImGui::TreeNodeEx("WAVE Text Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SliderFloat("Scale Ratio", &waveTextScaleRatio_, 0.1f, 2.0f);
		ImGui::SliderFloat("Offset X", &waveTextOffsetX_, -10.0f, 10.0f); // 🌟 X微調整用を追加
		ImGui::SliderFloat("Offset Y", &waveTextOffsetY_, -10.0f, 10.0f);
		ImGui::TreePop();
	}

	if (ImGui::Button("SAVE SETTINGS")) {
		GlobalVariables* global = GlobalVariables::GetInstance();
		const std::string groupName = "WaveUI";
		global->SetValue(groupName, "Color", Vector3(color_[0], color_[1], color_[2]));
		global->SetValue(groupName, "Intensity", intensity_);
		global->SetValue(groupName, "CenterX", centerX_);
		global->SetValue(groupName, "CenterY", centerY_);
		global->SetValue(groupName, "CenterScale", centerScale_);
		global->SetValue(groupName, "ShakeAmount", shakeAmount_);
		global->SetValue(groupName, "BottomRightX", bottomRightX_);
		global->SetValue(groupName, "BottomRightY", bottomRightY_);
		global->SetValue(groupName, "BottomRightScale", bottomRightScale_);
		global->SetValue(groupName, "TextSpacing", textSpacing_);
		global->SetValue(groupName, "RotY", rotY_);
		global->SetValue(groupName, "WaveTextScaleRatio", waveTextScaleRatio_);
		global->SetValue(groupName, "WaveTextOffsetX", waveTextOffsetX_); // 🌟 追加
		global->SetValue(groupName, "WaveTextOffsetY", waveTextOffsetY_);
		global->SaveFile(groupName);
	}

	ImGui::Separator();
	ImGui::Text("Current State: %s", (state_ == State::Center) ? "Center" : (state_ == State::Moving) ? "Moving" : "BottomRight");
	ImGui::Text("Display Wave: %d", displayWave_);

	ImGui::End();
#endif
}