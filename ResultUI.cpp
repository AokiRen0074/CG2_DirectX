#include "ResultUI.h"
#include "TextureManager.h"
#include "GlobalValiables.h"
#include <cmath>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void ResultUI::Initialize(const std::string& directoryPath, DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;

	mainTextModel_ = new NeonModel();
	mainTextModel_->Initialize(directoryPath, "UI_Clear_Text.obj");

	promptModel_ = new NeonModel();
	promptModel_->Initialize(directoryPath, "UI_PushSpace.obj");

	frameModel_ = new NeonModel();
	frameModel_->Initialize(directoryPath, "UI_Warning_Frame.obj");

	whiteTex_ = TextureManager::Load("Resources/white.png");
	state_ = ResultUIState();

	transformMain_.Initialize();
	transformPrompt_.Initialize();
	transformFrameTop_.Initialize();
	transformFrameBottom_.Initialize();

	neonScore_ = new NeonText();
	neonScore_->Initialize(dxCommon);

	neonTime_ = new NeonText();
	neonTime_->Initialize(dxCommon);

	// 通常のカメラ位置にリセット（Z=-60から正面を見る）
	uiViewProjection_.Initialize();
	uiViewProjection_.translation_ = { 0.0f, 2.0f, -60.0f };
	uiViewProjection_.rotation_ = { 0.0f, 0.0f, 0.0f };
	uiViewProjection_.UpdateMatrix();

	// JSON読み込み
	GlobalVariables* global = GlobalVariables::GetInstance();
	const std::string groupName = "ResultUI";
	global->CreateGroup(groupName);

	global->AddItem(groupName, "MainTextRotY", 0.0f);
	mainTextRotY_ = global->GetFloatValue(groupName, "MainTextRotY");

	global->AddItem(groupName, "MainTextColor", Vector3(0.0f, 1.0f, 1.0f));
	global->AddItem(groupName, "MainTextIntensity", 5.0f);
	global->AddItem(groupName, "MainTextOffsetY", 5.0f);
	global->AddItem(groupName, "MainTextScale", 1.0f);

	global->AddItem(groupName, "ScoreTextColor", Vector3(0.8f, 0.9f, 1.0f));
	global->AddItem(groupName, "ScoreTextIntensity", 3.0f);
	global->AddItem(groupName, "ScoreTextOffsetX", -5.0f);
	global->AddItem(groupName, "ScoreTextOffsetY", 1.0f);
	global->AddItem(groupName, "ScoreTextScale", 1.0f);
	global->AddItem(groupName, "TimeTextOffsetY", -3.0f);

	global->AddItem(groupName, "TextRadius", 0.01f);
	global->AddItem(groupName, "TextSoftness", 5.0f);
	global->AddItem(groupName, "TextLengthOffset", 0.0f);

	global->AddItem(groupName, "PromptColor", Vector3(1.0f, 1.0f, 1.0f));
	global->AddItem(groupName, "PromptIntensity", 3.0f);
	global->AddItem(groupName, "PromptOffsetY", -6.0f);
	global->AddItem(groupName, "PromptScale", 0.6f);

	global->AddItem(groupName, "FrameColor", Vector3(0.0f, 0.8f, 1.0f));
	global->AddItem(groupName, "FrameIntensity", 3.0f);
	global->AddItem(groupName, "FrameOffsetY", 9.0f);

	global->AddItem(groupName, "UIBaseZ", 0.0f);

	Vector3 mtc = global->GetVector3Value(groupName, "MainTextColor");
	mainTextColor_[0] = mtc.x; mainTextColor_[1] = mtc.y; mainTextColor_[2] = mtc.z;
	mainTextIntensity_ = global->GetFloatValue(groupName, "MainTextIntensity");
	mainTextOffsetY_ = global->GetFloatValue(groupName, "MainTextOffsetY");
	mainTextScale_ = global->GetFloatValue(groupName, "MainTextScale");

	Vector3 stc = global->GetVector3Value(groupName, "ScoreTextColor");
	scoreTextColor_[0] = stc.x; scoreTextColor_[1] = stc.y; scoreTextColor_[2] = stc.z;
	scoreTextIntensity_ = global->GetFloatValue(groupName, "ScoreTextIntensity");
	scoreTextOffsetX_ = global->GetFloatValue(groupName, "ScoreTextOffsetX");
	scoreTextOffsetY_ = global->GetFloatValue(groupName, "ScoreTextOffsetY");
	scoreTextScale_ = global->GetFloatValue(groupName, "ScoreTextScale");
	timeTextOffsetY_ = global->GetFloatValue(groupName, "TimeTextOffsetY");

	textRadius_ = global->GetFloatValue(groupName, "TextRadius");
	textSoftness_ = global->GetFloatValue(groupName, "TextSoftness");
	textLengthOffset_ = global->GetFloatValue(groupName, "TextLengthOffset");

	Vector3 pc = global->GetVector3Value(groupName, "PromptColor");
	promptColor_[0] = pc.x; promptColor_[1] = pc.y; promptColor_[2] = pc.z;
	promptIntensity_ = global->GetFloatValue(groupName, "PromptIntensity");
	promptOffsetY_ = global->GetFloatValue(groupName, "PromptOffsetY");
	promptScale_ = global->GetFloatValue(groupName, "PromptScale");

	Vector3 fc = global->GetVector3Value(groupName, "FrameColor");
	frameColor_[0] = fc.x; frameColor_[1] = fc.y; frameColor_[2] = fc.z;
	frameIntensity_ = global->GetFloatValue(groupName, "FrameIntensity");
	frameOffsetY_ = global->GetFloatValue(groupName, "FrameOffsetY");

	uiBaseZ_ = global->GetFloatValue(groupName, "UIBaseZ");
}

void ResultUI::Start(int score, float clearTime) {
	state_.isActive = true;
	state_.isFinished = false;
	state_.timer = 0.0f;
	state_.isExiting = false;
	state_.isExitFinished = false;
	state_.exitTimer = 0.0f;
	currentTextFade_ = 0.0f;
	finalScore_ = score;
	finalTime_ = clearTime;

	RebuildText();
}

void ResultUI::RebuildText() {
	if (neonScore_) { delete neonScore_; neonScore_ = nullptr; }
	if (neonTime_) { delete neonTime_; neonTime_ = nullptr; }

	neonScore_ = new NeonText();
	neonScore_->Initialize(dxCommon_);
	neonTime_ = new NeonText();
	neonTime_->Initialize(dxCommon_);

	std::string scoreStr = "SCORE: " + std::to_string(finalScore_);
	char timeStr[64];
	snprintf(timeStr, sizeof(timeStr), "TIME: %d SEC", static_cast<int>(finalTime_));

	float baseY = 2.0f;
	neonScore_->Print(scoreStr, scoreTextOffsetX_, baseY + scoreTextOffsetY_, scoreTextScale_);
	neonTime_->Print(timeStr, scoreTextOffsetX_, baseY + scoreTextOffsetY_ + timeTextOffsetY_, scoreTextScale_);
}

void ResultUI::Update(float deltaTime) {
	if (!state_.isActive) return;

	float baseY = 2.0f;
	float baseZ = uiBaseZ_;
	float currentScaleMain = 1.0f;
	float slideOffset = 0.0f;
	float promptScaleCurrent = promptScale_;

	if (!state_.isExiting) {
		state_.timer += deltaTime;
		if (state_.timer >= 2.0f) state_.isFinished = true;

		float currentTime = state_.timer;

		// メインテキスト
		float mainT = currentTime / animMainInTime_;
		if (mainT > 1.0f) mainT = 1.0f;
		currentScaleMain = 1.0f - std::pow(1.0f - mainT, 3.0f);

		// フレーム
		if (currentTime < 1.0f) {
			float t = currentTime / 1.0f;
			slideOffset = 18.0f * std::pow(1.0f - t, 3.0f);
		}

		// プロンプト
		float promptT = (currentTime - 1.5f);
		if (promptT < 0.0f) {
			promptScaleCurrent = 0.0f;
			promptBlink_ = 0.0f;
		}
		else {
			promptBlink_ = (std::sin(promptT * 4.0f) * 0.5f + 0.5f);
		}

		// スコア文字のフェード
		if (state_.timer >= 1.0f) {
			currentTextFade_ = (state_.timer - 1.0f) / 0.5f;
			if (currentTextFade_ > 1.0f) currentTextFade_ = 1.0f;
		}
		else {
			currentTextFade_ = 0.0f;
		}
	}
	else {
		state_.exitTimer += deltaTime;
		float exitTimeMax = 0.6f;
		float t = state_.exitTimer / exitTimeMax;
		if (t > 1.0f) {
			t = 1.0f;
			state_.isExitFinished = true; // 完全に退出完了
		}

		float easeExit = t * t * t; // 徐々に加速して消える

		currentScaleMain = 1.0f - easeExit;          // 縮んで消える
		slideOffset = easeExit * 30.0f;              // フレームが上下に開いて画面外へ
		promptScaleCurrent = promptScale_ * (1.0f - easeExit); 
		currentTextFade_ = 1.0f - easeExit;          // 文字はフェードアウト
	}

	// ==========================================
	// 適用処理
	// ==========================================
	transformMain_.scale_ = { currentScaleMain * mainTextScale_, currentScaleMain * mainTextScale_, currentScaleMain * mainTextScale_ };
	transformMain_.rotation_.y = mainTextRotY_;
	transformMain_.translation_ = { 0.0f, baseY + mainTextOffsetY_, baseZ };
	transformMain_.matWorld_ = MakeAffineMatrix(transformMain_.scale_, transformMain_.rotation_, transformMain_.translation_);
	transformMain_.TransferMatrix();

	float topY = frameOffsetY_ + slideOffset;
	float bottomY = -frameOffsetY_ - slideOffset;

	transformFrameTop_.translation_ = { 0.0f, baseY + topY, baseZ };
	transformFrameTop_.rotation_.y = 3.14159f;
	transformFrameTop_.scale_ = { 1.2f, 1.2f, 1.0f };
	transformFrameTop_.matWorld_ = MakeAffineMatrix(transformFrameTop_.scale_, transformFrameTop_.rotation_, transformFrameTop_.translation_);
	transformFrameTop_.TransferMatrix();

	transformFrameBottom_.translation_ = { 0.0f, baseY + bottomY, baseZ };
	transformFrameBottom_.rotation_.y = 3.14159f;
	transformFrameBottom_.rotation_.z = 3.14159f;
	transformFrameBottom_.scale_ = { 1.2f, 1.2f, 1.0f };
	transformFrameBottom_.matWorld_ = MakeAffineMatrix(transformFrameBottom_.scale_, transformFrameBottom_.rotation_, transformFrameBottom_.translation_);
	transformFrameBottom_.TransferMatrix();

	transformPrompt_.rotation_.y = mainTextRotY_;
	transformPrompt_.scale_ = { promptScaleCurrent, promptScaleCurrent, promptScaleCurrent };
	transformPrompt_.translation_ = { 0.0f, baseY + promptOffsetY_, baseZ };
	transformPrompt_.matWorld_ = MakeAffineMatrix(transformPrompt_.scale_, transformPrompt_.rotation_, transformPrompt_.translation_);
	transformPrompt_.TransferMatrix();

	if (currentTextFade_ > 0.0f && neonScore_ && neonTime_) {
		float currentIntensity = scoreTextIntensity_ * currentTextFade_;
		neonScore_->SetMaterial(textRadius_, textSoftness_, currentIntensity, scoreTextColor_[0], scoreTextColor_[1], scoreTextColor_[2], textLengthOffset_);
		neonScore_->Update(uiViewProjection_.matView, uiViewProjection_.matProjection);

		neonTime_->SetMaterial(textRadius_, textSoftness_, currentIntensity, scoreTextColor_[0], scoreTextColor_[1], scoreTextColor_[2], textLengthOffset_);
		neonTime_->Update(uiViewProjection_.matView, uiViewProjection_.matProjection);
	}
}

void ResultUI::Draw(const ViewProjection& viewProjection) {
	if (!state_.isActive) return;

	if (state_.timer >= 0.0f) {
		frameModel_->SetNeonColor(frameIntensity_, frameColor_[0], frameColor_[1], frameColor_[2]);
		frameModel_->Draw(transformFrameTop_, uiViewProjection_, whiteTex_);
		frameModel_->Draw(transformFrameBottom_, uiViewProjection_, whiteTex_);
	}

	if (transformMain_.scale_.x > 0.001f) {
		mainTextModel_->SetNeonColor(mainTextIntensity_, mainTextColor_[0], mainTextColor_[1], mainTextColor_[2]);
		mainTextModel_->Draw(transformMain_, uiViewProjection_, whiteTex_);
	}

	if (transformPrompt_.scale_.x > 0.001f) {
		float currentIntensity = promptIntensity_ * (state_.isExiting ? 1.0f : promptBlink_); // 退出時は点滅を止める
		promptModel_->SetNeonColor(currentIntensity, promptColor_[0], promptColor_[1], promptColor_[2]);
		promptModel_->Draw(transformPrompt_, uiViewProjection_, whiteTex_);
	}

	if (currentTextFade_ > 0.001f && neonScore_ && neonTime_) {
		neonScore_->Draw();
		neonTime_->Draw();
	}
}

// ==========================================
//  ImGui描画処理
// ==========================================
void ResultUI::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("Result UI Settings");

	if (ImGui::Button("PLAY RESULT (Test)")) {
		Start(9999, 45.2f);
	}

	ImGui::Separator();
	ImGui::Text("--- 3D Text Rotation (Flip Fix) ---");
	ImGui::SliderFloat("Main Text Rot Y", &mainTextRotY_, 0.0f, 6.28f);

	ImGui::Separator();
	ImGui::Text("--- Main Text Layout ---");
	ImGui::ColorEdit3("Main Text Color", mainTextColor_);
	ImGui::SliderFloat("Main Intensity", &mainTextIntensity_, 0.0f, 20.0f);
	ImGui::SliderFloat("Main Offset Y", &mainTextOffsetY_, -20.0f, 20.0f);
	ImGui::SliderFloat("Main Scale", &mainTextScale_, 0.1f, 5.0f);

	ImGui::Separator();
	ImGui::Text("--- Score & Time Text Layout ---");
	bool isRebuildNeeded = false;
	ImGui::ColorEdit3("Score Text Color", scoreTextColor_);
	ImGui::SliderFloat("Score Intensity", &scoreTextIntensity_, 0.0f, 20.0f);

	if (ImGui::SliderFloat("Score Offset X", &scoreTextOffsetX_, -40.0f, 40.0f)) isRebuildNeeded = true;
	if (ImGui::SliderFloat("Score Offset Y", &scoreTextOffsetY_, -20.0f, 20.0f)) isRebuildNeeded = true;
	if (ImGui::SliderFloat("Score Scale", &scoreTextScale_, 0.1f, 5.0f)) isRebuildNeeded = true;
	if (ImGui::SliderFloat("Time Offset Y", &timeTextOffsetY_, -10.0f, 10.0f)) isRebuildNeeded = true;

	ImGui::Separator();
	ImGui::Text("--- Neon Texture Tuning (Block Fix) ---");
	ImGui::SliderFloat("Text Radius", &textRadius_, 0.001f, 0.1f);
	ImGui::SliderFloat("Text Softness", &textSoftness_, 0.1f, 50.0f);
	ImGui::SliderFloat("Text Length Offset", &textLengthOffset_, -1.0f, 1.0f);

	ImGui::Separator();
	ImGui::Text("--- Frame Layout ---");
	ImGui::ColorEdit3("Frame Color", frameColor_);
	ImGui::SliderFloat("Frame Intensity", &frameIntensity_, 0.0f, 20.0f);
	ImGui::SliderFloat("Frame Offset Y", &frameOffsetY_, 0.0f, 20.0f);

	ImGui::Separator();
	ImGui::Text("--- Prompt Text Layout ---");
	ImGui::ColorEdit3("Prompt Color", promptColor_);
	ImGui::SliderFloat("Prompt Intensity", &promptIntensity_, 0.0f, 20.0f);
	ImGui::SliderFloat("Prompt Offset Y", &promptOffsetY_, -20.0f, 20.0f);
	ImGui::SliderFloat("Prompt Scale", &promptScale_, 0.1f, 5.0f);

	if (isRebuildNeeded && state_.isActive) {
		RebuildText();
	}

	if (ImGui::Button("SAVE SETTINGS")) {
		GlobalVariables* global = GlobalVariables::GetInstance();
		const std::string groupName = "ResultUI";
		global->SetValue(groupName, "MainTextRotY", mainTextRotY_);
		global->SetValue(groupName, "MainTextColor", Vector3(mainTextColor_[0], mainTextColor_[1], mainTextColor_[2]));
		global->SetValue(groupName, "MainTextIntensity", mainTextIntensity_);
		global->SetValue(groupName, "MainTextOffsetY", mainTextOffsetY_);
		global->SetValue(groupName, "MainTextScale", mainTextScale_);
		global->SetValue(groupName, "ScoreTextColor", Vector3(scoreTextColor_[0], scoreTextColor_[1], scoreTextColor_[2]));
		global->SetValue(groupName, "ScoreTextIntensity", scoreTextIntensity_);
		global->SetValue(groupName, "ScoreTextOffsetX", scoreTextOffsetX_);
		global->SetValue(groupName, "ScoreTextOffsetY", scoreTextOffsetY_);
		global->SetValue(groupName, "ScoreTextScale", scoreTextScale_);
		global->SetValue(groupName, "TimeTextOffsetY", timeTextOffsetY_);
		global->SetValue(groupName, "TextRadius", textRadius_);
		global->SetValue(groupName, "TextSoftness", textSoftness_);
		global->SetValue(groupName, "TextLengthOffset", textLengthOffset_);
		global->SetValue(groupName, "PromptColor", Vector3(promptColor_[0], promptColor_[1], promptColor_[2]));
		global->SetValue(groupName, "PromptIntensity", promptIntensity_);
		global->SetValue(groupName, "PromptOffsetY", promptOffsetY_);
		global->SetValue(groupName, "PromptScale", promptScale_);
		global->SetValue(groupName, "FrameColor", Vector3(frameColor_[0], frameColor_[1], frameColor_[2]));
		global->SetValue(groupName, "FrameIntensity", frameIntensity_);
		global->SetValue(groupName, "FrameOffsetY", frameOffsetY_);
		global->SaveFile(groupName);
	}

	ImGui::End();
#endif
}