#include "RebootUI.h"
#include "GlobalValiables.h"
#include <cmath>
#include <random>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

RebootUI::~RebootUI() {
	delete rebootModel_;
}

void RebootUI::Initialize(DirectXCommon* dxCommon, uint32_t whiteTex) {
	dxCommon_ = dxCommon;
	whiteTex_ = whiteTex;

	rebootModel_ = new NeonModel();
	rebootModel_->Initialize("Resources/UI", "reboot.obj");
	transform_.Initialize();

	uiViewProjection_.Initialize();
	uiViewProjection_.translation_ = { 0.0f, 0.0f, -50.0f };
	uiViewProjection_.UpdateMatrix();

	GlobalVariables* global = GlobalVariables::GetInstance();
	const std::string groupName = "RebootUI";
	global->CreateGroup(groupName);
	global->AddItem(groupName, "Color", Vector3(1.0f, 0.0f, 0.2f));
	global->AddItem(groupName, "Intensity", 8.0f);
	global->AddItem(groupName, "Scale", 2.0f);
	global->AddItem(groupName, "OffsetY", 5.0f);

	Vector3 c = global->GetVector3Value(groupName, "Color");
	color_[0] = c.x; color_[1] = c.y; color_[2] = c.z;
	baseIntensity_ = global->GetFloatValue(groupName, "Intensity");
	scale_ = global->GetFloatValue(groupName, "Scale");
	offsetY_ = global->GetFloatValue(groupName, "OffsetY");
}

void RebootUI::Start() {
	isActive_ = true;
	isFinished_ = false;
	timer_ = 0.0f;
}

void RebootUI::Update() {
	if (!isActive_) return;
	timer_ += 1.0f;

	// 🌟 バグったような点滅（グリッチ演出）
	currentIntensity_ = baseIntensity_ + std::sin(timer_ * 0.8f) * 4.0f;
	if (rand() % 100 < 15) {
		currentIntensity_ = 0.0f; // 15%の確率で一瞬真っ暗になる
	}

	transform_.scale_ = { scale_, scale_, scale_ };
	transform_.rotation_ = { 0.0f, 3.14159f, 0.0f }; // 反転補正
	transform_.translation_ = { 0.0f, offsetY_, 0.0f };
	transform_.matWorld_ = MakeAffineMatrix(transform_.scale_, transform_.rotation_, transform_.translation_);
	transform_.TransferMatrix();

	if (timer_ >= maxTime_) {
		isFinished_ = true;
		isActive_ = false;
	}
}

void RebootUI::Draw() {
	if (!isActive_) return;
	if (rebootModel_) {
		rebootModel_->SetNeonColor(currentIntensity_, color_[0], color_[1], color_[2]);
		rebootModel_->Draw(transform_, uiViewProjection_, whiteTex_);
	}
}

void RebootUI::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("Reboot UI Settings");
	ImGui::ColorEdit3("Color", color_);
	ImGui::SliderFloat("Intensity", &baseIntensity_, 0.0f, 20.0f);
	ImGui::SliderFloat("Scale", &scale_, 0.1f, 10.0f);
	ImGui::SliderFloat("Offset Y", &offsetY_, -20.0f, 20.0f);

	if (ImGui::Button("SAVE SETTINGS")) {
		GlobalVariables* global = GlobalVariables::GetInstance();
		const std::string groupName = "RebootUI";
		global->SetValue(groupName, "Color", Vector3(color_[0], color_[1], color_[2]));
		global->SetValue(groupName, "Intensity", baseIntensity_);
		global->SetValue(groupName, "Scale", scale_);
		global->SetValue(groupName, "OffsetY", offsetY_);
		global->SaveFile(groupName);
	}
	if (ImGui::Button("Test Reboot Anime")) { Start(); }
	ImGui::End();
#endif
}