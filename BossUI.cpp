#include "BossUI..h"
#include "TextureManager.h"
#include "GlobalValiables.h"
#include <cmath>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

BossUI::~BossUI() {
	delete barModel_;
	delete frameModel_;
}

void BossUI::Initialize() {
	barModel_ = new NeonModel();
	barModel_->Initialize("Resources", "block.obj");

	frameModel_ = new NeonModel();
	frameModel_->Initialize("Resources", "block.obj");

	whiteTex_ = TextureManager::Load("Resources/white.png");

	barTransform_.Initialize();
	frameTransform_.Initialize();

	uiViewProjection_.Initialize();
	uiViewProjection_.translation_ = { 0.0f, 0.0f, -50.0f };
	uiViewProjection_.UpdateMatrix();


	GlobalVariables* global = GlobalVariables::GetInstance();
	const std::string groupName = "BossUI";
	global->CreateGroup(groupName);
	global->AddItem(groupName, "MaxWidth", barMaxWidth_);
	global->AddItem(groupName, "Height", barHeight_);
	global->AddItem(groupName, "PosX", barPosX_);
	global->AddItem(groupName, "PosY", barPosY_);
	global->AddItem(groupName, "FrameColor", Vector3(frameColor_[0], frameColor_[1], frameColor_[2]));
	global->AddItem(groupName, "FrameIntensity", frameIntensity_);
	global->AddItem(groupName, "BarColor", Vector3(barColor_[0], barColor_[1], barColor_[2]));
	global->AddItem(groupName, "BarIntensity", barIntensity_);

	barMaxWidth_ = global->GetFloatValue(groupName, "MaxWidth");
	barHeight_ = global->GetFloatValue(groupName, "Height");
	barPosX_ = global->GetFloatValue(groupName, "PosX");
	barPosY_ = global->GetFloatValue(groupName, "PosY");
	Vector3 fc = global->GetVector3Value(groupName, "FrameColor");
	frameColor_[0] = fc.x; frameColor_[1] = fc.y; frameColor_[2] = fc.z;
	frameIntensity_ = global->GetFloatValue(groupName, "FrameIntensity");
	Vector3 bc = global->GetVector3Value(groupName, "BarColor");
	barColor_[0] = bc.x; barColor_[1] = bc.y; barColor_[2] = bc.z;
	barIntensity_ = global->GetFloatValue(groupName, "BarIntensity");
}

void BossUI::Update(int currentHp, int maxHp, bool isBattleStarted) {
	if (!isBattleStarted && !isStarted_) { 
		appearTimer_ = 0.0f;
		displayRatio_ = 0.0f;
		return;
	}

	if (!isStarted_) isStarted_ = true;

	if (appearTimer_ < 1.0f) {
		appearTimer_ += 0.02f;
		if (appearTimer_ > 1.0f) appearTimer_ = 1.0f;
		float ease = 1.0f - std::pow(1.0f - appearTimer_, 3.0f);
		displayRatio_ = ease;
	}
	else {
		float targetRatio = (float)currentHp / (float)maxHp;
		if (targetRatio < 0.0f) targetRatio = 0.0f;
		displayRatio_ += (targetRatio - displayRatio_) * 0.1f;
	}

	frameTransform_.scale_ = { barMaxWidth_ + 0.3f, barHeight_ + 0.3f, 1.0f };
	frameTransform_.translation_ = { barPosX_, barPosY_, 0.0f };
	frameTransform_.matWorld_ = MakeAffineMatrix(frameTransform_.scale_, frameTransform_.rotation_, frameTransform_.translation_);
	frameTransform_.TransferMatrix();

	float currentWidth = barMaxWidth_ * displayRatio_;
	float offsetX = (barMaxWidth_ - currentWidth) * 0.5f;

	barTransform_.scale_ = { currentWidth, barHeight_, 1.0f };
	barTransform_.translation_ = { barPosX_ - offsetX, barPosY_, -0.1f };
	barTransform_.matWorld_ = MakeAffineMatrix(barTransform_.scale_, barTransform_.rotation_, barTransform_.translation_);
	barTransform_.TransferMatrix();
}

void BossUI::Draw() {
	if (!isStarted_) return;

	if (frameModel_) {
		frameModel_->SetNeonColor(frameIntensity_, frameColor_[0], frameColor_[1], frameColor_[2]);
		frameModel_->Draw(frameTransform_, uiViewProjection_, whiteTex_);
	}

	if (barModel_) {
		barModel_->SetNeonColor(barIntensity_, barColor_[0], barColor_[1], barColor_[2]);
		barModel_->Draw(barTransform_, uiViewProjection_, whiteTex_);
	}
}

void BossUI::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("Boss UI Settings");


	ImGui::Checkbox("Force Show (Test)", &isStarted_);
	if (isStarted_ && appearTimer_ == 0.0f) appearTimer_ = 1.0f; // 強制表示時はすぐ伸びる
	ImGui::Separator();

	ImGui::SliderFloat("Pos X", &barPosX_, -40.0f, 40.0f);
	ImGui::SliderFloat("Pos Y", &barPosY_, -40.0f, 40.0f);
	ImGui::SliderFloat("Max Width", &barMaxWidth_, 10.0f, 80.0f);
	ImGui::SliderFloat("Height", &barHeight_, 0.1f, 5.0f);

	ImGui::Separator();
	ImGui::ColorEdit3("Frame Color", frameColor_);
	ImGui::SliderFloat("Frame Intensity", &frameIntensity_, 0.0f, 20.0f);
	ImGui::ColorEdit3("Bar Color", barColor_);
	ImGui::SliderFloat("Bar Intensity", &barIntensity_, 0.0f, 30.0f);

	if (ImGui::Button("SAVE SETTINGS")) {
		GlobalVariables* global = GlobalVariables::GetInstance();
		const std::string groupName = "BossUI";
		global->SetValue(groupName, "MaxWidth", barMaxWidth_);
		global->SetValue(groupName, "Height", barHeight_);
		global->SetValue(groupName, "PosX", barPosX_);
		global->SetValue(groupName, "PosY", barPosY_);
		global->SetValue(groupName, "FrameColor", Vector3(frameColor_[0], frameColor_[1], frameColor_[2]));
		global->SetValue(groupName, "FrameIntensity", frameIntensity_);
		global->SetValue(groupName, "BarColor", Vector3(barColor_[0], barColor_[1], barColor_[2]));
		global->SetValue(groupName, "BarIntensity", barIntensity_);
		global->SaveFile(groupName);
	}
	ImGui::End();
#endif
}