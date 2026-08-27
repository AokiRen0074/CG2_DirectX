#include "ScoreUI.h"
#include "GlobalValiables.h"
#include <cmath>
#include <algorithm>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

ScoreUI::~ScoreUI() {
	for (int i = 0; i < 10; ++i) {
		delete numberModels_[i];
		delete popupNumberModels_[i]; 
	}
	delete plusModel_;
}

void ScoreUI::Initialize(DirectXCommon* dxCommon, uint32_t whiteTex) {
	dxCommon_ = dxCommon;
	whiteTex_ = whiteTex;

	for (int i = 0; i < 10; ++i) {
		std::string fileName = std::to_string(i) + ".obj";

		// メインスコア用
		numberModels_[i] = new NeonModel();
		numberModels_[i]->Initialize("Resources/UI", fileName);

		// ポップアップ専用
		popupNumberModels_[i] = new NeonModel();
		popupNumberModels_[i]->Initialize("Resources/UI", fileName);
	}

	// ＋マークの読み込み
	plusModel_ = new NeonModel();
	plusModel_->Initialize("Resources/UI", "plus.obj");
	plusTransform_.Initialize();

	for (int i = 0; i < kMaxDigits; ++i) {
		digitTransforms_[i].Initialize();
		popupTransforms_[i].Initialize();
	}

	uiViewProjection_.Initialize();
	uiViewProjection_.translation_ = { 0.0f, 0.0f, -50.0f };
	uiViewProjection_.UpdateMatrix();

	GlobalVariables* global = GlobalVariables::GetInstance();
	const std::string groupName = "ScoreUI";
	global->CreateGroup(groupName);
	global->AddItem(groupName, "TextColor", Vector3(1.0f, 1.0f, 1.0f));
	global->AddItem(groupName, "TextIntensity", 6.0f);
	global->AddItem(groupName, "TextOffsetX", -5.0f);
	global->AddItem(groupName, "TextOffsetY", -15.0f);
	global->AddItem(groupName, "TextScale", 1.0f);
	global->AddItem(groupName, "TextSpacing", 2.0f);
	global->AddItem(groupName, "TextRotY", 3.14159f);

	global->AddItem(groupName, "PopupColor", Vector3(1.0f, 0.8f, 0.0f));
	global->AddItem(groupName, "PopupIntensity", 8.0f);
	global->AddItem(groupName, "PopupOffsetY", 3.0f);
	global->AddItem(groupName, "PopupScale", 0.7f);

	Vector3 tc = global->GetVector3Value(groupName, "TextColor");
	textColor_[0] = tc.x; textColor_[1] = tc.y; textColor_[2] = tc.z;
	textIntensity_ = global->GetFloatValue(groupName, "TextIntensity");
	textOffsetX_ = global->GetFloatValue(groupName, "TextOffsetX");
	textOffsetY_ = global->GetFloatValue(groupName, "TextOffsetY");
	textScale_ = global->GetFloatValue(groupName, "TextScale");
	textSpacing_ = global->GetFloatValue(groupName, "TextSpacing");
	textRotY_ = global->GetFloatValue(groupName, "TextRotY");

	Vector3 pc = global->GetVector3Value(groupName, "PopupColor");
	popupColor_[0] = pc.x; popupColor_[1] = pc.y; popupColor_[2] = pc.z;
	popupIntensity_ = global->GetFloatValue(groupName, "PopupIntensity");
	popupOffsetY_ = global->GetFloatValue(groupName, "PopupOffsetY");
	popupScale_ = global->GetFloatValue(groupName, "PopupScale");

	// 音
	scoreSound_ = Audio::GetInstance()->SoundLoadWave("Sounds/Score.wav");
}

void ScoreUI::Update() {
	if (currentDisplayScore_ < targetScore_) {
		float diff = targetScore_ - currentDisplayScore_;

		if (diff < 1.5f) {
			currentDisplayScore_ = (float)targetScore_;
		}
		else {
			currentDisplayScore_ += (std::max)(1.0f, diff * 0.1f);
		}

		if (currentDisplayScore_ > targetScore_) currentDisplayScore_ = (float)targetScore_;

	
		if (scoreVoice_ == nullptr) {
			scoreVoice_ = Audio::GetInstance()->SoundPlayWave(scoreSound_);
			soundDelayTimer_ = 60; // 音がしっかり鳴り切るまでの猶予フレーム
		}
	}
	else {
		// カウントアップが終わっても、余韻タイマーが残っている間は自然に鳴らし続ける
		if (soundDelayTimer_ > 0) {
			soundDelayTimer_--;
		}
		else {
			if (scoreVoice_) {
				Audio::GetInstance()->SoundStopWave(scoreVoice_);
				scoreVoice_ = nullptr;
			}
		}
	}

	char buffer[32];
	snprintf(buffer, sizeof(buffer), "%06d", (int)currentDisplayScore_);
	currentDisplayString_ = buffer;

	// メインスコアのTransform
	for (size_t i = 0; i < currentDisplayString_.length() && i < kMaxDigits; ++i) {
		float drawX = textOffsetX_ + (i * textSpacing_ * textScale_);
		digitTransforms_[i].scale_ = { textScale_, textScale_, textScale_ };
		digitTransforms_[i].rotation_ = { 0.0f, textRotY_, 0.0f };
		digitTransforms_[i].translation_ = { drawX, textOffsetY_, 0.0f };
		digitTransforms_[i].matWorld_ = MakeAffineMatrix(digitTransforms_[i].scale_, digitTransforms_[i].rotation_, digitTransforms_[i].translation_);
		digitTransforms_[i].TransferMatrix();
	}

	if (popupTimer_ > 0.0f) {
		popupTimer_ -= 1.0f;
	}
	else {
		popupScore_ = 0;
	}


	if (popupScore_ > 0) {
		std::string popupString = std::to_string(popupScore_);

		float mainWidth = (kMaxDigits - 1) * textSpacing_ * textScale_;
		float centerX = textOffsetX_ + mainWidth * 0.5f;

		// ＋記号分も含めて長さを計算
		int totalPopupLen = static_cast<int>(popupString.length()) + 1;
		float popupWidth = (totalPopupLen - 1) * textSpacing_ * popupScale_;
		float popupStartX = centerX - popupWidth * 0.5f;

		//  ＋ 記号の座標を設定
		plusTransform_.scale_ = { popupScale_, popupScale_, popupScale_ };
		plusTransform_.rotation_ = { 0.0f, textRotY_, 0.0f };
		plusTransform_.translation_ = { popupStartX, textOffsetY_ + popupOffsetY_, 0.0f };
		plusTransform_.matWorld_ = MakeAffineMatrix(plusTransform_.scale_, plusTransform_.rotation_, plusTransform_.translation_);
		plusTransform_.TransferMatrix();

		//  数字は ＋ 記号の隣から並べていく
		float numberStartX = popupStartX + (textSpacing_ * popupScale_);
		for (size_t i = 0; i < popupString.length() && i < kMaxDigits; ++i) {
			float drawX = numberStartX + (i * textSpacing_ * popupScale_);

			popupTransforms_[i].scale_ = { popupScale_, popupScale_, popupScale_ };
			popupTransforms_[i].rotation_ = { 0.0f, textRotY_, 0.0f };
			popupTransforms_[i].translation_ = { drawX, textOffsetY_ + popupOffsetY_, 0.0f };
			popupTransforms_[i].matWorld_ = MakeAffineMatrix(popupTransforms_[i].scale_, popupTransforms_[i].rotation_, popupTransforms_[i].translation_);
			popupTransforms_[i].TransferMatrix();
		}
	}
}

void ScoreUI::Draw() {
	// メインスコア
	for (size_t i = 0; i < currentDisplayString_.length() && i < kMaxDigits; ++i) {
		char c = currentDisplayString_[i];
		int num = c - '0';
		if (num >= 0 && num <= 9) {
			NeonModel* model = numberModels_[num]; // メイン用モデル
			model->SetNeonColor(textIntensity_, textColor_[0], textColor_[1], textColor_[2]);
			model->Draw(digitTransforms_[i], uiViewProjection_, whiteTex_);
		}
	}

	//  ポップアップスコア
	if (popupTimer_ > 0.0f && popupScore_ > 0) {
		float alpha = popupTimer_ / popupMaxTime_;
		float currentIntensity = popupIntensity_ * (alpha * alpha); // シュッと消えるイージング

		//  ＋記号を描画
		if (plusModel_) {
			plusModel_->SetNeonColor(currentIntensity, popupColor_[0], popupColor_[1], popupColor_[2]);
			plusModel_->Draw(plusTransform_, uiViewProjection_, whiteTex_);
		}

		std::string popupString = std::to_string(popupScore_);
		for (size_t i = 0; i < popupString.length() && i < kMaxDigits; ++i) {
			char c = popupString[i];
			int num = c - '0';
			if (num >= 0 && num <= 9) {
				NeonModel* model = popupNumberModels_[num]; // ポップアップ専用モデル
				model->SetNeonColor(currentIntensity, popupColor_[0], popupColor_[1], popupColor_[2]);
				model->Draw(popupTransforms_[i], uiViewProjection_, whiteTex_);
			}
		}
	}
}

void ScoreUI::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("Score UI Settings");

	if (ImGui::TreeNodeEx("Main Score Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::ColorEdit3("Text Color", textColor_);
		ImGui::SliderFloat("Text Intensity", &textIntensity_, 0.0f, 20.0f);
		ImGui::SliderFloat("Text Offset X", &textOffsetX_, -40.0f, 40.0f);
		ImGui::SliderFloat("Text Offset Y", &textOffsetY_, -40.0f, 40.0f);
		ImGui::SliderFloat("Text Scale", &textScale_, 0.1f, 5.0f);
		ImGui::SliderFloat("Text Spacing", &textSpacing_, 0.1f, 5.0f);
		ImGui::SliderFloat("Text Rot Y", &textRotY_, 0.0f, 6.28f);
		ImGui::TreePop();
	}

	ImGui::Separator();

	if (ImGui::TreeNodeEx("Popup Score Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::ColorEdit3("Popup Color", popupColor_);
		ImGui::SliderFloat("Popup Intensity", &popupIntensity_, 0.0f, 20.0f);
		ImGui::SliderFloat("Popup Offset Y", &popupOffsetY_, -20.0f, 20.0f);
		ImGui::SliderFloat("Popup Scale", &popupScale_, 0.1f, 5.0f);
		ImGui::TreePop();
	}

	if (ImGui::Button("SAVE SETTINGS")) {
		GlobalVariables* global = GlobalVariables::GetInstance();
		const std::string groupName = "ScoreUI";
		global->SetValue(groupName, "TextColor", Vector3(textColor_[0], textColor_[1], textColor_[2]));
		global->SetValue(groupName, "TextIntensity", textIntensity_);
		global->SetValue(groupName, "TextOffsetX", textOffsetX_);
		global->SetValue(groupName, "TextOffsetY", textOffsetY_);
		global->SetValue(groupName, "TextScale", textScale_);
		global->SetValue(groupName, "TextSpacing", textSpacing_);
		global->SetValue(groupName, "TextRotY", textRotY_);

		global->SetValue(groupName, "PopupColor", Vector3(popupColor_[0], popupColor_[1], popupColor_[2]));
		global->SetValue(groupName, "PopupIntensity", popupIntensity_);
		global->SetValue(groupName, "PopupOffsetY", popupOffsetY_);
		global->SetValue(groupName, "PopupScale", popupScale_);
		global->SaveFile(groupName);
	}

	ImGui::Separator();
	ImGui::Text("Target Score: %d", targetScore_);
	ImGui::Text("Display Score: %d", (int)currentDisplayScore_);
	ImGui::Text("Popup Score: %d", popupScore_);
	if (ImGui::Button("Test +150 Score")) { AddScore(150); }

	ImGui::End();
#endif
}