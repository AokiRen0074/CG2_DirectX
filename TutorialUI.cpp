#include "TutorialUI.h"
#include "GlobalValiables.h"
#include "Input/Input.h"
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

TutorialUI::~TutorialUI() {
	delete moveBase_;
	delete moveNeon_;
	delete shootBase_;
	delete shootNeon_;
}

void TutorialUI::Initialize(DirectXCommon* dxCommon, uint32_t whiteTex, uint32_t blackTex) {
	dxCommon_ = dxCommon;
	whiteTex_ = whiteTex;
	blackTex_ = blackTex;

	// モデルの読み込み
	moveBase_ = new NeonModel();
	moveBase_->Initialize("Resources/UI", "tutorial_move_base.obj");
	moveNeon_ = new NeonModel();
	moveNeon_->Initialize("Resources/UI", "tutorial_move_neon.obj");

	shootBase_ = new NeonModel();
	shootBase_->Initialize("Resources/UI", "tutorial_shoot_base.obj");
	shootNeon_ = new NeonModel();
	shootNeon_->Initialize("Resources/UI", "tutorial_shoot_neon.obj");

	moveTransform_.Initialize();
	shootTransform_.Initialize();

	uiViewProjection_.Initialize();
	uiViewProjection_.translation_ = { 0.0f, 0.0f, -50.0f };
	uiViewProjection_.UpdateMatrix();

	GlobalVariables* global = GlobalVariables::GetInstance();
	const std::string groupName = "TutorialUI";
	global->CreateGroup(groupName);
	global->AddItem(groupName, "Color", Vector3(0.0f, 0.8f, 1.0f));
	global->AddItem(groupName, "Intensity", 8.0f);
	global->AddItem(groupName, "CenterScale", 2.0f);
	global->AddItem(groupName, "CornerScale", 0.8f);
	global->AddItem(groupName, "RotX", -1.57f);
	global->AddItem(groupName, "RotY", 0.0f);
	global->AddItem(groupName, "RotZ", 0.0f);
	global->AddItem(groupName, "CenterX", 0.0f);
	global->AddItem(groupName, "CenterY", 5.0f);
	global->AddItem(groupName, "CornerMoveX", -8.0f);
	global->AddItem(groupName, "CornerMoveY", -7.0f);
	global->AddItem(groupName, "CornerShootX", -10.0f);
	global->AddItem(groupName, "CornerShootY", 3.0f);

	Vector3 c = global->GetVector3Value(groupName, "Color");
	color_[0] = c.x; color_[1] = c.y; color_[2] = c.z;
	intensity_ = global->GetFloatValue(groupName, "Intensity");
	centerScale_ = global->GetFloatValue(groupName, "CenterScale");
	cornerScale_ = global->GetFloatValue(groupName, "CornerScale");
	rotX_ = global->GetFloatValue(groupName, "RotX");
	rotY_ = global->GetFloatValue(groupName, "RotY");
	rotZ_ = global->GetFloatValue(groupName, "RotZ");
	centerX_ = global->GetFloatValue(groupName, "CenterX");
	centerY_ = global->GetFloatValue(groupName, "CenterY");
	cornerMoveX_ = global->GetFloatValue(groupName, "CornerMoveX");
	cornerMoveY_ = global->GetFloatValue(groupName, "CornerMoveY");
	cornerShootX_ = global->GetFloatValue(groupName, "CornerShootX");
	cornerShootY_ = global->GetFloatValue(groupName, "CornerShootY");
}

void TutorialUI::Update(int currentWave, bool isInterval) {
	// Wave1の戦闘が始まった瞬間にチュートリアル開始
	if (currentWave == 1 && !isInterval && state_ == State::Hidden) {
		state_ = State::Appear;
		timer_ = 0.0f;
		isMoveCompleted_ = false;
		isShootCompleted_ = false;
		moveShrinkT_ = 0.0f;
		shootShrinkT_ = 0.0f;
	}

	// Waveが2以降になったら完全に非表示
	if (currentWave > 1) {
		state_ = State::Hidden;
		return;
	}

	if (state_ == State::Hidden) return;

	Input* input = Input::GetInstance();

	// 🌟 アクション検知！
	if (state_ == State::Active) {
		// 移動キーのどれかが押されたら
		if (input->PushKey(DIK_W) || input->PushKey(DIK_A) || input->PushKey(DIK_S) || input->PushKey(DIK_D) ||
			input->PushKey(DIK_UP) || input->PushKey(DIK_DOWN) || input->PushKey(DIK_LEFT) || input->PushKey(DIK_RIGHT)) {
			isMoveCompleted_ = true;
		}
		// スペースキーが押されたら
		if (input->PushKey(DIK_SPACE)) {
			isShootCompleted_ = true;
		}
	}

	float targetMoveX = centerX_, targetMoveY = centerY_;
	float targetShootX = centerX_ + 10.0f, targetShootY = centerY_; // 初期は横並び
	float targetScale = 0.0f;

	if (state_ == State::Appear) {
		timer_ += 1.0f;
		float t = timer_ / appearMaxTime_;
		float tMinus1 = t - 1.0f;
		float easeT = 1.0f + 2.70158f * std::pow(tMinus1, 3.0f) + 1.70158f * std::pow(tMinus1, 2.0f);
		targetScale = centerScale_ * easeT;

		if (timer_ >= appearMaxTime_) {
			state_ = State::MoveToCorner;
			timer_ = 0.0f;
		}
	}
	else if (state_ == State::MoveToCorner) {
		timer_ += 1.0f;
		float t = timer_ / moveMaxTime_;
		if (t >= 1.0f) { t = 1.0f; state_ = State::Active; }
		float easeT = 1.0f - std::pow(1.0f - t, 3.0f);

		targetMoveX = Lerp(centerX_, cornerMoveX_, easeT);
		targetMoveY = Lerp(centerY_, cornerMoveY_, easeT);
		targetShootX = Lerp(centerX_ + 10.0f, cornerShootX_, easeT);
		targetShootY = Lerp(centerY_, cornerShootY_, easeT);
		targetScale = Lerp(centerScale_, cornerScale_, easeT);
	}
	else if (state_ == State::Active) {
		swayTimer_ += 1.0f / 60.0f;
		float swayY = std::sin(swayTimer_ * 2.0f) * swayAmount_;

		targetMoveX = cornerMoveX_;
		targetMoveY = cornerMoveY_ + swayY;
		targetShootX = cornerShootX_;
		targetShootY = cornerShootY_ + std::cos(swayTimer_ * 2.0f) * swayAmount_; // タイミングを少しズラす
		targetScale = cornerScale_;
	}

	// 🌟 縮小アニメーションの計算
	if (isMoveCompleted_) {
		moveShrinkT_ += 0.05f; // シュッと消える速度
		if (moveShrinkT_ > 1.0f) moveShrinkT_ = 1.0f;
	}
	if (isShootCompleted_) {
		shootShrinkT_ += 0.05f;
		if (shootShrinkT_ > 1.0f) shootShrinkT_ = 1.0f;
	}

	// EaseInBackで少し膨らんでからシュッと消える魔法の計算式
	auto easeInBack = [](float x) { return 2.70158f * x * x * x - 1.70158f * x * x; };
	float finalMoveScale = targetScale * (1.0f - easeInBack(moveShrinkT_));
	float finalShootScale = targetScale * (1.0f - easeInBack(shootShrinkT_));

	if (finalMoveScale < 0.0f) finalMoveScale = 0.0f;
	if (finalShootScale < 0.0f) finalShootScale = 0.0f;

	Vector3 rotation = { rotX_, rotY_, rotZ_ };

	moveTransform_.scale_ = { finalMoveScale, finalMoveScale, finalMoveScale };
	moveTransform_.translation_ = { targetMoveX, targetMoveY, 0.0f };
	moveTransform_.matWorld_ = MakeAffineMatrix(moveTransform_.scale_, rotation, moveTransform_.translation_); // 変更
	moveTransform_.TransferMatrix();

	shootTransform_.scale_ = { finalShootScale, finalShootScale, finalShootScale };
	shootTransform_.translation_ = { targetShootX, targetShootY, 0.0f };
	shootTransform_.matWorld_ = MakeAffineMatrix(shootTransform_.scale_, rotation, shootTransform_.translation_); // 変更
	shootTransform_.TransferMatrix();
}

void TutorialUI::DrawBase() {
	if (state_ == State::Hidden) return;

	if (moveBase_ && moveShrinkT_ < 1.0f) {
		moveBase_->SetNeonColor(0.0f, 0.0f, 0.0f, 1.0f);
		moveBase_->Draw(moveTransform_, uiViewProjection_, blackTex_);
	}
	if (shootBase_ && shootShrinkT_ < 1.0f) {
		shootBase_->SetNeonColor(0.0f, 0.0f, 0.0f, 1.0f);
		shootBase_->Draw(shootTransform_, uiViewProjection_, blackTex_);
	}
}

void TutorialUI::DrawNeon() {
	if (state_ == State::Hidden) return;

	// ネオン文字を少し手前に浮かせる
	WorldTransform moveNeonTransform = moveTransform_;
	moveNeonTransform.translation_.z -= 0.5f;
	moveNeonTransform.matWorld_ = MakeAffineMatrix(moveNeonTransform.scale_, { rotX_, rotY_, rotZ_ }, moveNeonTransform.translation_);
	moveNeonTransform.TransferMatrix();

	WorldTransform shootNeonTransform = shootTransform_;
	shootNeonTransform.translation_.z -= 0.5f;
	shootNeonTransform.matWorld_ = MakeAffineMatrix(shootNeonTransform.scale_, { rotX_, rotY_, rotZ_ }, shootNeonTransform.translation_);
	shootNeonTransform.TransferMatrix();

	// ネオンを描画！
	if (moveNeon_ && moveShrinkT_ < 1.0f) {
		moveNeon_->SetNeonColor(intensity_, color_[0], color_[1], color_[2]);
		moveNeon_->Draw(moveNeonTransform, uiViewProjection_, whiteTex_);
	}
	if (shootNeon_ && shootShrinkT_ < 1.0f) {
		shootNeon_->SetNeonColor(intensity_, color_[0], color_[1], color_[2]);
		shootNeon_->Draw(shootNeonTransform, uiViewProjection_, whiteTex_);
	}
}

void TutorialUI::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("Tutorial UI Settings");
	ImGui::ColorEdit3("Neon Color", color_);
	ImGui::SliderFloat("Intensity", &intensity_, 0.0f, 20.0f);

	ImGui::SliderFloat("Center X", &centerX_, -40.0f, 40.0f);
	ImGui::SliderFloat("Center Y", &centerY_, -40.0f, 40.0f);

	ImGui::Separator();
	ImGui::Text("Corner Positions (Left Bottom)");
	ImGui::SliderFloat("Move X", &cornerMoveX_, -40.0f, 40.0f);
	ImGui::SliderFloat("Move Y", &cornerMoveY_, -40.0f, 40.0f);
	ImGui::SliderFloat("Shoot X", &cornerShootX_, -40.0f, 40.0f);
	ImGui::SliderFloat("Shoot Y", &cornerShootY_, -40.0f, 40.0f);

	ImGui::Separator();

	ImGui::Text("Rotation Settings");
	ImGui::SliderFloat("Rot X", &rotX_, -3.14f, 3.14f);
	ImGui::SliderFloat("Rot Y", &rotY_, -3.14f, 3.14f);
	ImGui::SliderFloat("Rot Z", &rotZ_, -3.14f, 3.14f);

	if (ImGui::Button("Reset Tutorial")) {
		state_ = State::Hidden;
	}
	if (ImGui::Button("SAVE SETTINGS")) {
		GlobalVariables* global = GlobalVariables::GetInstance();
		global->SetValue("TutorialUI", "Color", Vector3(color_[0], color_[1], color_[2]));
		global->SetValue("TutorialUI", "Intensity", intensity_);
		global->SetValue("TutorialUI", "CenterScale", centerScale_);
		global->SetValue("TutorialUI", "CornerScale", cornerScale_);

		// 👇 追加：角度と座標もすべて保存リストに送る！
		global->SetValue("TutorialUI", "RotX", rotX_);
		global->SetValue("TutorialUI", "RotY", rotY_);
		global->SetValue("TutorialUI", "RotZ", rotZ_);

		global->SetValue("TutorialUI", "CenterX", centerX_);
		global->SetValue("TutorialUI", "CenterY", centerY_);
		global->SetValue("TutorialUI", "CornerMoveX", cornerMoveX_);
		global->SetValue("TutorialUI", "CornerMoveY", cornerMoveY_);
		global->SetValue("TutorialUI", "CornerShootX", cornerShootX_);
		global->SetValue("TutorialUI", "CornerShootY", cornerShootY_);

		global->SaveFile("TutorialUI"); // ここで初めて一斉保存される
	}
	ImGui::End();
#endif
}