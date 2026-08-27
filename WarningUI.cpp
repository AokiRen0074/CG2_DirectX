#include "WarningUI.h"
#include "TextureManager.h"
#include "GlobalValiables.h"
#include <cmath>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

WarningUI::~WarningUI() {
	delete modelFrame_;
	delete modelText_;
	delete modelIcon_;
}

void WarningUI::Initialize(const std::string& directoryPath) {
	modelFrame_ = new NeonModel();
	modelFrame_->Initialize(directoryPath, "UI_Warning_Frame.obj");

	modelText_ = new NeonModel();
	modelText_->Initialize(directoryPath, "UI_Warning_Text.obj");

	modelIcon_ = new NeonModel();
	modelIcon_->Initialize(directoryPath, "UI_Warning_Icon.obj");

	transformFrameTop_.Initialize();
	transformFrameBottom_.Initialize();
	transformText_.Initialize();
	transformIconL_.Initialize();
	transformIconR_.Initialize();

	uiViewProjection_.Initialize();
	uiViewProjection_.translation_ = { 0.0f, 0.0f, -60.0f };
	uiViewProjection_.rotation_ = { 0.0f, 0.0f, 0.0f };
	uiViewProjection_.UpdateMatrix();

	whiteTexture_ = TextureManager::Load("Resources/white.png");

	isActive_ = false;
	isFinished_ = false;
	timer_ = 0.0f;
	animeTime_ = 0.0f;
	isDebugKeepActive_ = false;

	// ==========================================
	// JSONからのデータ読み込み
	// ==========================================
	GlobalVariables* global = GlobalVariables::GetInstance();
	const std::string groupName = "WarningUI";
	global->CreateGroup(groupName);

	global->AddItem(groupName, "TextColor", Vector3(1.0f, 0.0f, 0.0f));
	global->AddItem(groupName, "TextBaseIntensity", 2.0f);
	global->AddItem(groupName, "TextFlashIntensity", 15.0f);
	global->AddItem(groupName, "FrameColor", Vector3(0.8f, 0.8f, 0.8f));
	global->AddItem(groupName, "FrameIntensity", 2.0f);
	global->AddItem(groupName, "FrameFlashColor", Vector3(1.0f, 1.0f, 1.0f));
	global->AddItem(groupName, "FrameFlashIntensity", 30.0f);

	global->AddItem(groupName, "FrameScaleX", 1.0f);
	global->AddItem(groupName, "FrameScaleY", 1.0f);
	global->AddItem(groupName, "FrameOffsetY", 7.0f);
	global->AddItem(groupName, "TextOffsetX", 0.0f);
	global->AddItem(groupName, "TextOffsetY", 0.0f);
	global->AddItem(groupName, "TextScale", 1.0f);
	global->AddItem(groupName, "IconOffsetX", 12.0f);
	global->AddItem(groupName, "IconOffsetY", 0.0f);
	global->AddItem(groupName, "IconScale", 0.8f);

	Vector3 tc = global->GetVector3Value(groupName, "TextColor");
	textColor_[0] = tc.x; textColor_[1] = tc.y; textColor_[2] = tc.z;
	textBaseIntensity_ = global->GetFloatValue(groupName, "TextBaseIntensity");
	textFlashIntensity_ = global->GetFloatValue(groupName, "TextFlashIntensity");

	Vector3 fc = global->GetVector3Value(groupName, "FrameColor");
	frameColor_[0] = fc.x; frameColor_[1] = fc.y; frameColor_[2] = fc.z;
	frameIntensity_ = global->GetFloatValue(groupName, "FrameIntensity");

	Vector3 ffc = global->GetVector3Value(groupName, "FrameFlashColor");
	frameFlashColor_[0] = ffc.x; frameFlashColor_[1] = ffc.y; frameFlashColor_[2] = ffc.z;
	frameFlashIntensity_ = global->GetFloatValue(groupName, "FrameFlashIntensity");

	frameScaleX_ = global->GetFloatValue(groupName, "FrameScaleX");
	frameScaleY_ = global->GetFloatValue(groupName, "FrameScaleY");
	frameOffsetY_ = global->GetFloatValue(groupName, "FrameOffsetY");
	textOffsetX_ = global->GetFloatValue(groupName, "TextOffsetX");
	textOffsetY_ = global->GetFloatValue(groupName, "TextOffsetY");
	textScale_ = global->GetFloatValue(groupName, "TextScale");
	iconOffsetX_ = global->GetFloatValue(groupName, "IconOffsetX");
	iconOffsetY_ = global->GetFloatValue(groupName, "IconOffsetY");
	iconScale_ = global->GetFloatValue(groupName, "IconScale");
}

void WarningUI::StartWarning() {
	isActive_ = true;
	isFinished_ = false;
	timer_ = 0.0f;
	animeTime_ = 0.0f;
}

void WarningUI::Update() {
	if (isDebugKeepActive_) {
		isActive_ = true;
		isFinished_ = false;
		timer_ = 2.0f;
	}

	if (!isActive_) return;

	if (!isDebugKeepActive_) {
		timer_ += 1.0f / 60.0f;
	}

	animeTime_ += 0.1f;

	if (!isDebugKeepActive_ && timer_ >= 3.8f) {
		isActive_ = false;
		isFinished_ = true;
		return;
	}

	Vector3 uiCenter = { 0.0f, 0.0f, 0.0f };
	float rotY = 3.14159f;

	// ------------------------------------------------
	// 枠組みのスライドイン・アウト
	// ------------------------------------------------
	float slideOffset = 0.0f;

	if (timer_ < 0.8f) {
		float t = timer_ / 0.8f;
		slideOffset = 18.0f * std::pow(1.0f - t, 3.0f);
	}
	else if (timer_ >= 3.0f) {
		float t = (timer_ - 3.0f) / 0.8f;
		if (t > 1.0f) t = 1.0f;
		slideOffset = 18.0f * std::pow(t, 3.0f);
	}

	float topY = frameOffsetY_ + slideOffset;
	float bottomY = -frameOffsetY_ - slideOffset;

	transformFrameTop_.translation_ = { uiCenter.x, uiCenter.y + topY, uiCenter.z };
	transformFrameTop_.rotation_.y = rotY;
	transformFrameTop_.scale_ = { frameScaleX_, frameScaleY_, 1.0f };

	transformFrameBottom_.translation_ = { uiCenter.x, uiCenter.y + bottomY, uiCenter.z };
	transformFrameBottom_.rotation_.y = rotY;
	transformFrameBottom_.rotation_.z = 3.14159f;
	transformFrameBottom_.scale_ = { frameScaleX_, frameScaleY_, 1.0f };

	// ------------------------------------------------
	// 合体フラッシュ
	// ------------------------------------------------
	if (timer_ >= 0.8f && timer_ < 1.0f) {
		modelFrame_->SetNeonColor(frameFlashIntensity_, frameFlashColor_[0], frameFlashColor_[1], frameFlashColor_[2]);
	}
	else {
		modelFrame_->SetNeonColor(frameIntensity_, frameColor_[0], frameColor_[1], frameColor_[2]);
	}

	// ------------------------------------------------
	// 文字とアイコンのイージング点滅表示
	// ------------------------------------------------
	if (timer_ >= 1.0f && timer_ < 3.0f) {
		float blink = (std::sin(animeTime_ * 6.0f) * 0.5f + 0.5f);
		float currentIntensity = textBaseIntensity_ + (blink * (textFlashIntensity_ - textBaseIntensity_));

		modelText_->SetNeonColor(currentIntensity, textColor_[0], textColor_[1], textColor_[2]);
		modelIcon_->SetNeonColor(currentIntensity, textColor_[0], textColor_[1], textColor_[2]);

		//スケールのイージング計算
		float textTime = timer_ - 1.0f; // 0.0 ~ 2.0 の時間
		float scaleAnim = 1.0f;         // 最終的なスケール倍率

		if (textTime < 0.2f) {

			float t = textTime / 0.2f;
			scaleAnim = 1.0f - std::pow(1.0f - t, 3.0f);
		}
		else if (textTime > 1.8f) {
			// 【退場時】最後の0.2秒で、1.0から0.0へ滑らかに縮小
			float t = (textTime - 1.8f) / 0.2f;
			scaleAnim = 1.0f - std::pow(t, 3.0f);
		}

		// ImGuiのスケール設定値に、イージングの倍率を掛け合わせる
		transformText_.scale_ = { textScale_ * scaleAnim, textScale_ * scaleAnim, textScale_ * scaleAnim };
		transformIconL_.scale_ = { iconScale_ * scaleAnim, iconScale_ * scaleAnim, iconScale_ * scaleAnim };
		transformIconR_.scale_ = { iconScale_ * scaleAnim, iconScale_ * scaleAnim, iconScale_ * scaleAnim };
	}
	else {
		transformText_.scale_ = { 0.0f, 0.0f, 0.0f };
		transformIconL_.scale_ = { 0.0f, 0.0f, 0.0f };
		transformIconR_.scale_ = { 0.0f, 0.0f, 0.0f };
	}

	transformText_.translation_ = { uiCenter.x + textOffsetX_, uiCenter.y + textOffsetY_, uiCenter.z };
	transformText_.rotation_.y = rotY;

	transformIconL_.translation_ = { uiCenter.x - iconOffsetX_, uiCenter.y + iconOffsetY_, uiCenter.z };
	transformIconL_.rotation_.y = rotY;

	transformIconR_.translation_ = { uiCenter.x + iconOffsetX_, uiCenter.y + iconOffsetY_, uiCenter.z };
	transformIconR_.rotation_.y = rotY;

	// ==========================================
	// 行列転送
	// ==========================================
	transformFrameTop_.matWorld_ = MakeAffineMatrix(transformFrameTop_.scale_, transformFrameTop_.rotation_, transformFrameTop_.translation_);
	transformFrameTop_.TransferMatrix();

	transformFrameBottom_.matWorld_ = MakeAffineMatrix(transformFrameBottom_.scale_, transformFrameBottom_.rotation_, transformFrameBottom_.translation_);
	transformFrameBottom_.TransferMatrix();

	transformText_.matWorld_ = MakeAffineMatrix(transformText_.scale_, transformText_.rotation_, transformText_.translation_);
	transformText_.TransferMatrix();

	transformIconL_.matWorld_ = MakeAffineMatrix(transformIconL_.scale_, transformIconL_.rotation_, transformIconL_.translation_);
	transformIconL_.TransferMatrix();

	transformIconR_.matWorld_ = MakeAffineMatrix(transformIconR_.scale_, transformIconR_.rotation_, transformIconR_.translation_);
	transformIconR_.TransferMatrix();
}

void WarningUI::Draw() {
	if (!isActive_) return;

	modelFrame_->Draw(transformFrameTop_, uiViewProjection_, whiteTexture_);
	modelFrame_->Draw(transformFrameBottom_, uiViewProjection_, whiteTexture_);
	modelText_->Draw(transformText_, uiViewProjection_, whiteTexture_);
	modelIcon_->Draw(transformIconL_, uiViewProjection_, whiteTexture_);
	modelIcon_->Draw(transformIconR_, uiViewProjection_, whiteTexture_);
}

// ==========================================
// ImGui描画処理
// ==========================================
void WarningUI::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("Warning UI Settings");

	ImGui::Text("--- Debug & Play ---");
	if (ImGui::Button("PLAY WARNING (Test)")) {
		StartWarning();
		isDebugKeepActive_ = false;
	}
	ImGui::SameLine();
	ImGui::Checkbox("Keep UI Active (For Layout)", &isDebugKeepActive_);

	ImGui::Separator();
	ImGui::Text("--- Frame Layout ---");
	ImGui::SliderFloat("Frame Scale X", &frameScaleX_, 0.1f, 3.0f);
	ImGui::SliderFloat("Frame Scale Y", &frameScaleY_, 0.1f, 3.0f);
	ImGui::SliderFloat("Frame Offset Y", &frameOffsetY_, 0.0f, 20.0f);

	ImGui::Separator();
	ImGui::Text("--- Text Layout ---");
	ImGui::SliderFloat("Text Offset X", &textOffsetX_, -20.0f, 20.0f);
	ImGui::SliderFloat("Text Offset Y", &textOffsetY_, -20.0f, 20.0f);
	ImGui::SliderFloat("Text Scale", &textScale_, 0.1f, 3.0f);

	ImGui::Separator();
	ImGui::Text("--- Icon Layout ---");
	ImGui::SliderFloat("Icon Offset X (Distance)", &iconOffsetX_, 0.0f, 30.0f);
	ImGui::SliderFloat("Icon Offset Y", &iconOffsetY_, -20.0f, 20.0f);
	ImGui::SliderFloat("Icon Scale", &iconScale_, 0.1f, 3.0f);

	ImGui::Separator();
	ImGui::Text("--- Colors & Intensity ---");
	ImGui::ColorEdit3("Text Color", textColor_);
	ImGui::SliderFloat("Text Base Intensity", &textBaseIntensity_, 0.0f, 10.0f);
	ImGui::SliderFloat("Text Flash Intensity", &textFlashIntensity_, 0.0f, 50.0f);

	ImGui::ColorEdit3("Frame Color", frameColor_);
	ImGui::SliderFloat("Frame Intensity", &frameIntensity_, 0.0f, 10.0f);
	ImGui::ColorEdit3("Frame Flash Color", frameFlashColor_);
	ImGui::SliderFloat("Frame Flash Intensity", &frameFlashIntensity_, 0.0f, 50.0f);

	if (ImGui::Button("SAVE WARNING UI SETTINGS")) {
		GlobalVariables* global = GlobalVariables::GetInstance();
		const std::string groupName = "WarningUI";

		global->SetValue(groupName, "TextColor", Vector3(textColor_[0], textColor_[1], textColor_[2]));
		global->SetValue(groupName, "TextBaseIntensity", textBaseIntensity_);
		global->SetValue(groupName, "TextFlashIntensity", textFlashIntensity_);

		global->SetValue(groupName, "FrameColor", Vector3(frameColor_[0], frameColor_[1], frameColor_[2]));
		global->SetValue(groupName, "FrameIntensity", frameIntensity_);
		global->SetValue(groupName, "FrameFlashColor", Vector3(frameFlashColor_[0], frameFlashColor_[1], frameFlashColor_[2]));
		global->SetValue(groupName, "FrameFlashIntensity", frameFlashIntensity_);

		global->SetValue(groupName, "FrameScaleX", frameScaleX_);
		global->SetValue(groupName, "FrameScaleY", frameScaleY_);
		global->SetValue(groupName, "FrameOffsetY", frameOffsetY_);
		global->SetValue(groupName, "TextOffsetX", textOffsetX_);
		global->SetValue(groupName, "TextOffsetY", textOffsetY_);
		global->SetValue(groupName, "TextScale", textScale_);
		global->SetValue(groupName, "IconOffsetX", iconOffsetX_);
		global->SetValue(groupName, "IconOffsetY", iconOffsetY_);
		global->SetValue(groupName, "IconScale", iconScale_);

		global->SaveFile(groupName);
	}

	ImGui::End();
#endif
}