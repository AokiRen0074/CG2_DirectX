#include "LifeUI.h"
#include "GlobalValiables.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

LifeUI::~LifeUI() {
	delete lifeModel_;
}

void LifeUI::Initialize(DirectXCommon* dxCommon, uint32_t whiteTex) {
	dxCommon_ = dxCommon;
	whiteTex_ = whiteTex;


	// 🌟 作成した life.obj を読み込む
	lifeModel_ = new NeonModel();
	lifeModel_->Initialize("Resources/UI", "life.obj");

	for (int i = 0; i < kMaxLives; ++i) {
		lifeTransforms_[i].Initialize();
	}

	uiViewProjection_.Initialize();
	uiViewProjection_.translation_ = { 0.0f, 0.0f, -50.0f };
	uiViewProjection_.UpdateMatrix();

	GlobalVariables* global = GlobalVariables::GetInstance();
	const std::string groupName = "LifeUI";
	global->CreateGroup(groupName);
	global->AddItem(groupName, "IconColor", Vector3(0.0f, 0.8f, 1.0f));
	global->AddItem(groupName, "IconIntensity", 6.0f);
	global->AddItem(groupName, "IconOffsetX", 0.0f);
	global->AddItem(groupName, "IconOffsetY", -22.0f);
	global->AddItem(groupName, "IconScale", 0.8f);
	global->AddItem(groupName, "IconSpacing", 2.0f);
	global->AddItem(groupName, "IconRotY", 0.0f);

	Vector3 ic = global->GetVector3Value(groupName, "IconColor");
	iconColor_[0] = ic.x; iconColor_[1] = ic.y; iconColor_[2] = ic.z;
	iconIntensity_ = global->GetFloatValue(groupName, "IconIntensity");
	iconOffsetX_ = global->GetFloatValue(groupName, "IconOffsetX");
	iconOffsetY_ = global->GetFloatValue(groupName, "IconOffsetY");
	iconScale_ = global->GetFloatValue(groupName, "IconScale");
	iconSpacing_ = global->GetFloatValue(groupName, "IconSpacing");
	iconRotY_ = global->GetFloatValue(groupName, "IconRotY");
}

void LifeUI::Update() {
	// 🌟 アイコン全体が画面の中央に来るようにスタート位置を計算
	float totalWidth = (currentLife_ - 1) * iconSpacing_ * iconScale_;
	float startX = iconOffsetX_ - totalWidth * 0.5f;

	for (int i = 0; i < currentLife_; ++i) {
		float drawX = startX + (i * iconSpacing_ * iconScale_);

		lifeTransforms_[i].scale_ = { iconScale_, iconScale_, iconScale_ };
		lifeTransforms_[i].rotation_ = { 0.0f, iconRotY_, 0.0f };
		lifeTransforms_[i].translation_ = { drawX, iconOffsetY_, 0.0f };
		lifeTransforms_[i].matWorld_ = MakeAffineMatrix(lifeTransforms_[i].scale_, lifeTransforms_[i].rotation_, lifeTransforms_[i].translation_);
		lifeTransforms_[i].TransferMatrix();
	}
}

void LifeUI::Draw() {
	for (int i = 0; i < currentLife_; ++i) {
		lifeModel_->SetNeonColor(iconIntensity_, iconColor_[0], iconColor_[1], iconColor_[2]);
		lifeModel_->Draw(lifeTransforms_[i], uiViewProjection_, whiteTex_);
	}
}

void LifeUI::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("Life UI Settings");

	ImGui::ColorEdit3("Icon Color", iconColor_);
	ImGui::SliderFloat("Icon Intensity", &iconIntensity_, 0.0f, 20.0f);
	ImGui::SliderFloat("Icon Offset X", &iconOffsetX_, -40.0f, 40.0f);
	ImGui::SliderFloat("Icon Offset Y", &iconOffsetY_, -40.0f, 40.0f);
	ImGui::SliderFloat("Icon Scale", &iconScale_, 0.1f, 5.0f);
	ImGui::SliderFloat("Icon Spacing", &iconSpacing_, 0.1f, 5.0f);
	ImGui::SliderFloat("Icon Rot Y", &iconRotY_, 0.0f, 6.28f);

	if (ImGui::Button("SAVE SETTINGS")) {
		GlobalVariables* global = GlobalVariables::GetInstance();
		const std::string groupName = "LifeUI";
		global->SetValue(groupName, "IconColor", Vector3(iconColor_[0], iconColor_[1], iconColor_[2]));
		global->SetValue(groupName, "IconIntensity", iconIntensity_);
		global->SetValue(groupName, "IconOffsetX", iconOffsetX_);
		global->SetValue(groupName, "IconOffsetY", iconOffsetY_);
		global->SetValue(groupName, "IconScale", iconScale_);
		global->SetValue(groupName, "IconSpacing", iconSpacing_);
		global->SetValue(groupName, "IconRotY", iconRotY_);
		global->SaveFile(groupName);
	}

	ImGui::Separator();
	ImGui::Text("Current Life: %d", currentLife_);
	if (ImGui::Button("Test -1 Life")) { DecreaseLife(); }

	ImGui::End();
#endif
}