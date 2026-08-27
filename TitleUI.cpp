#include "TitleUI.h"
#include "TextureManager.h"
#include "GlobalValiables.h"
#include <cmath>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

// 🌟 デストラクタを追加してメモリリークを防ぐ
TitleUI::~TitleUI() {
	delete titleModel_;
	delete promptText_;
}

void TitleUI::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	isActive_ = true;
	timer_ = 0.0f;

	whiteTex_ = TextureManager::Load("Resources/white.png");

	uiViewProjection_.Initialize();
	uiViewProjection_.translation_ = { 0.0f, 0.0f, -60.0f };
	uiViewProjection_.rotation_ = { 0.0f, 0.0f, 0.0f };
	uiViewProjection_.UpdateMatrix();

	// 🌟 Title.obj の読み込み！
	titleModel_ = new NeonModel();
	titleModel_->Initialize("Resources/UI", "Title.obj");
	titleTransform_.Initialize();

	GlobalVariables* global = GlobalVariables::GetInstance();
	const std::string groupName = "TitleUI";
	global->CreateGroup(groupName);

	global->AddItem(groupName, "TitleColor", Vector3(0.0f, 1.0f, 1.0f));
	global->AddItem(groupName, "TitleIntensity", 5.0f);
	global->AddItem(groupName, "TitleOffsetX", 0.0f);
	global->AddItem(groupName, "TitleOffsetY", 2.0f);
	global->AddItem(groupName, "TitleOffsetZ", 0.0f);
	global->AddItem(groupName, "TitleScale", 1.0f);
	global->AddItem(groupName, "TitleRotX", 0.0f);
	global->AddItem(groupName, "TitleRotY", 0.0f);

	global->AddItem(groupName, "PromptColor", Vector3(1.0f, 1.0f, 1.0f));
	global->AddItem(groupName, "PromptIntensity", 3.0f);
	global->AddItem(groupName, "PromptOffsetX", -7.5f);
	global->AddItem(groupName, "PromptOffsetY", -3.0f);
	global->AddItem(groupName, "PromptScale", 0.8f);

	global->AddItem(groupName, "TextRadius", 0.01f);
	global->AddItem(groupName, "TextSoftness", 5.0f);
	global->AddItem(groupName, "TextLengthOffset", 0.0f);

	Vector3 tc = global->GetVector3Value(groupName, "TitleColor");
	titleColor_[0] = tc.x; titleColor_[1] = tc.y; titleColor_[2] = tc.z;
	titleIntensity_ = global->GetFloatValue(groupName, "TitleIntensity");
	titleOffsetX_ = global->GetFloatValue(groupName, "TitleOffsetX");
	titleOffsetY_ = global->GetFloatValue(groupName, "TitleOffsetY");
	titleOffsetZ_ = global->GetFloatValue(groupName, "TitleOffsetZ");
	titleScale_ = global->GetFloatValue(groupName, "TitleScale");
	titleRotX_ = global->GetFloatValue(groupName, "TitleRotX");
	titleRotY_ = global->GetFloatValue(groupName, "TitleRotY");

	Vector3 pc = global->GetVector3Value(groupName, "PromptColor");
	promptColor_[0] = pc.x; promptColor_[1] = pc.y; promptColor_[2] = pc.z;
	promptIntensity_ = global->GetFloatValue(groupName, "PromptIntensity");
	promptOffsetX_ = global->GetFloatValue(groupName, "PromptOffsetX");
	promptOffsetY_ = global->GetFloatValue(groupName, "PromptOffsetY");
	promptScale_ = global->GetFloatValue(groupName, "PromptScale");

	textRadius_ = global->GetFloatValue(groupName, "TextRadius");
	textSoftness_ = global->GetFloatValue(groupName, "TextSoftness");
	textLengthOffset_ = global->GetFloatValue(groupName, "TextLengthOffset");

	RebuildText();
}

void TitleUI::RebuildText() {
	if (promptText_) { delete promptText_; promptText_ = nullptr; }
	promptText_ = new NeonText();
	promptText_->Initialize(dxCommon_);
	promptText_->Print(promptString_, promptOffsetX_, promptOffsetY_, promptScale_);
}

void TitleUI::Update(float deltaTime) {
	if (!isActive_) return;
	timer_ += deltaTime;

	float offsetMain = 0.0f;
	float offsetPrompt = 0.0f;

	if (isStarting_) {
		startTimer_ += deltaTime;
		float t = startTimer_ / 1.0f;
		if (t > 1.0f) t = 1.0f;
		float ease = t * t * t;

		offsetMain = ease * 30.0f;
		offsetPrompt = ease * 30.0f;
	}

	// 🌟 ロゴモデルのトランスフォームを更新
	if (titleModel_) {
		// フワフワ浮かせる演出
		float floatY = std::sin(timer_ * 2.0f) * 0.5f;

		titleTransform_.scale_ = { titleScale_, titleScale_, titleScale_ };
		titleTransform_.rotation_ = { titleRotX_, titleRotY_, 0.0f };
		titleTransform_.translation_ = { titleOffsetX_, titleOffsetY_ + offsetMain + floatY, titleOffsetZ_ };

		titleTransform_.matWorld_ = MakeAffineMatrix(titleTransform_.scale_, titleTransform_.rotation_, titleTransform_.translation_);
		titleTransform_.TransferMatrix();
	}

	if (promptText_) {
		ViewProjection promptVP = uiViewProjection_;
		promptVP.translation_.y += offsetPrompt;
		promptVP.UpdateMatrix();

		float blinkSpeed = isStarting_ ? 30.0f : 5.0f;
		float blink = std::sin(timer_ * blinkSpeed) * 0.5f + 0.5f;
		float currentIntensity = promptIntensity_ * blink;

		promptText_->SetMaterial(textRadius_, textSoftness_, currentIntensity, promptColor_[0], promptColor_[1], promptColor_[2], textLengthOffset_);
		promptText_->Update(promptVP.matView, promptVP.matProjection);
	}
}

void TitleUI::Draw() {
	if (!isActive_) return;

	// 🌟 ロゴモデルの描画！
	if (titleModel_) {
		titleModel_->SetNeonColor(titleIntensity_, titleColor_[0], titleColor_[1], titleColor_[2]);
		titleModel_->Draw(titleTransform_, uiViewProjection_, whiteTex_);
	}

	if (promptText_) promptText_->Draw();
}

void TitleUI::DrawImGui() {
#ifdef USE_IMGUI
	if (!isActive_) return;
	ImGui::Begin("Title UI Settings");

	bool isRebuildNeeded = false;

	ImGui::Separator();
	ImGui::Text("--- Title Logo Model (Title.obj) ---");
	ImGui::ColorEdit3("Logo Color", titleColor_);
	ImGui::SliderFloat("Logo Intensity", &titleIntensity_, 0.0f, 30.0f);

	// 🌟 スケールや回転、Z座標も調整可能に！
	ImGui::SliderFloat("Logo Pos X", &titleOffsetX_, -40.0f, 40.0f);
	ImGui::SliderFloat("Logo Pos Y", &titleOffsetY_, -40.0f, 40.0f);
	ImGui::SliderFloat("Logo Pos Z", &titleOffsetZ_, -100.0f, 100.0f);
	ImGui::SliderFloat("Logo Scale", &titleScale_, 0.1f, 10.0f);
	ImGui::SliderFloat("Logo Rot X", &titleRotX_, -3.14f, 3.14f);
	ImGui::SliderFloat("Logo Rot Y", &titleRotY_, -3.14f, 3.14f);

	ImGui::Separator();
	ImGui::Text("--- Prompt Text (PUSH SPACE) ---");

	if (ImGui::InputText("Prompt Text", promptString_, sizeof(promptString_))) isRebuildNeeded = true;

	ImGui::ColorEdit3("Prompt Color", promptColor_);
	ImGui::SliderFloat("Prompt Intensity", &promptIntensity_, 0.0f, 20.0f);
	if (ImGui::SliderFloat("Prompt Offset X", &promptOffsetX_, -40.0f, 40.0f)) isRebuildNeeded = true;
	if (ImGui::SliderFloat("Prompt Offset Y", &promptOffsetY_, -40.0f, 40.0f)) isRebuildNeeded = true;
	if (ImGui::SliderFloat("Prompt Scale", &promptScale_, 0.1f, 5.0f)) isRebuildNeeded = true;

	ImGui::Separator();
	ImGui::Text("--- Neon Texture Tuning (Prompt Only) ---");
	ImGui::SliderFloat("Text Radius", &textRadius_, 0.001f, 0.1f);
	ImGui::SliderFloat("Text Softness", &textSoftness_, 0.1f, 50.0f);
	ImGui::SliderFloat("Text Length Offset", &textLengthOffset_, -1.0f, 1.0f);

	if (isRebuildNeeded) RebuildText();

	if (ImGui::Button("SAVE SETTINGS")) {
		GlobalVariables* global = GlobalVariables::GetInstance();
		const std::string groupName = "TitleUI";

		global->SetValue(groupName, "TitleColor", Vector3(titleColor_[0], titleColor_[1], titleColor_[2]));
		global->SetValue(groupName, "TitleIntensity", titleIntensity_);
		global->SetValue(groupName, "TitleOffsetX", titleOffsetX_);
		global->SetValue(groupName, "TitleOffsetY", titleOffsetY_);
		global->SetValue(groupName, "TitleOffsetZ", titleOffsetZ_);
		global->SetValue(groupName, "TitleScale", titleScale_);
		global->SetValue(groupName, "TitleRotX", titleRotX_);
		global->SetValue(groupName, "TitleRotY", titleRotY_);

		global->SetValue(groupName, "PromptColor", Vector3(promptColor_[0], promptColor_[1], promptColor_[2]));
		global->SetValue(groupName, "PromptIntensity", promptIntensity_);
		global->SetValue(groupName, "PromptOffsetX", promptOffsetX_);
		global->SetValue(groupName, "PromptOffsetY", promptOffsetY_);
		global->SetValue(groupName, "PromptScale", promptScale_);

		global->SetValue(groupName, "TextRadius", textRadius_);
		global->SetValue(groupName, "TextSoftness", textSoftness_);
		global->SetValue(groupName, "TextLengthOffset", textLengthOffset_);

		global->SaveFile(groupName);
	}
	ImGui::End();
#endif
}