#include "TitleUI.h"
#include "GlobalValiables.h"
#include <cmath>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void TitleUI::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
	isActive_ = true;
	timer_ = 0.0f;

	uiViewProjection_.Initialize();
	uiViewProjection_.translation_ = { 0.0f, 0.0f, -60.0f }; // 60.0f だったのを -60.0f に変更！
	uiViewProjection_.rotation_ = { 0.0f, 0.0f, 0.0f };      // 3.14159f だったのを 0.0f に変更！
	uiViewProjection_.UpdateMatrix();

	GlobalVariables* global = GlobalVariables::GetInstance();
	const std::string groupName = "TitleUI";
	global->CreateGroup(groupName);

	global->AddItem(groupName, "TitleColor", Vector3(0.0f, 1.0f, 1.0f));
	global->AddItem(groupName, "TitleIntensity", 5.0f);
	global->AddItem(groupName, "TitleOffsetX", -3.5f);
	global->AddItem(groupName, "TitleOffsetY", 2.0f);
	global->AddItem(groupName, "TitleScale", 2.0f);

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
	titleScale_ = global->GetFloatValue(groupName, "TitleScale");

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
	if (titleText_) { delete titleText_; titleText_ = nullptr; }
	if (promptText_) { delete promptText_; promptText_ = nullptr; }

	titleText_ = new NeonText();
	titleText_->Initialize(dxCommon_);
	promptText_ = new NeonText();
	promptText_->Initialize(dxCommon_);

	// 🌟 修正：直接入力した変数から文字を読み込む！
	titleText_->Print(titleString_, titleOffsetX_, titleOffsetY_, titleScale_);
	promptText_->Print(promptString_, promptOffsetX_, promptOffsetY_, promptScale_);
}
void TitleUI::Update(float deltaTime) {
	if (!isActive_) return;
	timer_ += deltaTime;

	float offsetMain = 0.0f;
	float offsetPrompt = 0.0f;

	// 🌟 スタートが押されたら、上下に割れるように画面外へ飛ばす！
	if (isStarting_) {
		startTimer_ += deltaTime;
		float t = startTimer_ / 1.0f; // 1秒かけて飛ばす
		if (t > 1.0f) t = 1.0f;
		float ease = t * t * t; // 3乗イージング（徐々に加速）

		offsetMain = ease * 30.0f;   // TITLEは上へ
		offsetPrompt = ease * 30.0f; // PUSH SPACEは下へ
	}

	if (titleText_) {
		ViewProjection mainVP = uiViewProjection_;
		mainVP.translation_.y -= offsetMain; // カメラを下げる＝文字は上に行く
		mainVP.UpdateMatrix();

		titleText_->SetMaterial(textRadius_, textSoftness_, titleIntensity_, titleColor_[0], titleColor_[1], titleColor_[2], textLengthOffset_);
		titleText_->Update(mainVP.matView, mainVP.matProjection);
	}

	if (promptText_) {
		ViewProjection promptVP = uiViewProjection_;
		promptVP.translation_.y += offsetPrompt; // カメラを上げる＝文字は下に行く
		promptVP.UpdateMatrix();

		// スタート時はピカピカ！と激しく点滅させる
		float blinkSpeed = isStarting_ ? 30.0f : 5.0f;
		float blink = std::sin(timer_ * blinkSpeed) * 0.5f + 0.5f;
		float currentIntensity = promptIntensity_ * blink;

		promptText_->SetMaterial(textRadius_, textSoftness_, currentIntensity, promptColor_[0], promptColor_[1], promptColor_[2], textLengthOffset_);
		promptText_->Update(promptVP.matView, promptVP.matProjection);
	}
}

void TitleUI::Draw() {
	if (!isActive_) return;
	if (titleText_) titleText_->Draw();
	if (promptText_) promptText_->Draw();
}

void TitleUI::DrawImGui() {
#ifdef USE_IMGUI
	if (!isActive_) return;
	ImGui::Begin("Title UI Settings");

	bool isRebuildNeeded = false;

	ImGui::Separator();
	ImGui::Text("--- Title Text Layout ---");

	// 🌟 追加：ここで好きなタイトルを打ち込めます！
	if (ImGui::InputText("Title Text", titleString_, sizeof(titleString_))) isRebuildNeeded = true;

	ImGui::ColorEdit3("Title Color", titleColor_);
	ImGui::SliderFloat("Title Intensity", &titleIntensity_, 0.0f, 20.0f);
	if (ImGui::SliderFloat("Title Offset X", &titleOffsetX_, -20.0f, 20.0f)) isRebuildNeeded = true;
	if (ImGui::SliderFloat("Title Offset Y", &titleOffsetY_, -20.0f, 20.0f)) isRebuildNeeded = true;
	if (ImGui::SliderFloat("Title Scale", &titleScale_, 0.1f, 10.0f)) isRebuildNeeded = true;

	ImGui::Separator();
	ImGui::Text("--- Prompt Text Layout ---");

	// 🌟 追加：こちらも文字変更可能に！
	if (ImGui::InputText("Prompt Text", promptString_, sizeof(promptString_))) isRebuildNeeded = true;

	ImGui::ColorEdit3("Prompt Color", promptColor_);
	ImGui::SliderFloat("Prompt Intensity", &promptIntensity_, 0.0f, 20.0f);
	if (ImGui::SliderFloat("Prompt Offset X", &promptOffsetX_, -20.0f, 20.0f)) isRebuildNeeded = true;
	if (ImGui::SliderFloat("Prompt Offset Y", &promptOffsetY_, -20.0f, 20.0f)) isRebuildNeeded = true;
	if (ImGui::SliderFloat("Prompt Scale", &promptScale_, 0.1f, 5.0f)) isRebuildNeeded = true;

	ImGui::Separator();
	ImGui::Text("--- Neon Texture Tuning ---");
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
		global->SetValue(groupName, "TitleScale", titleScale_);
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