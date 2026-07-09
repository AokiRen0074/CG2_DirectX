#include "NeonObj.h"
#include "Matrix4x4.h" // MakeAffineMatrix を使うため
#include <cmath>
#include <cstdlib>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

NeonObj::~NeonObj() {
	delete model_;
}

void NeonObj::Initialize(const std::string& directoryPath, const std::string& filename) {
	model_ = new NeonModel();
	model_->Initialize(directoryPath, filename);

	transform_.Initialize();
	// 念のためダミーテクスチャを読み込む
	dummyTexture_ = TextureManager::Load("Resources/uvChecker.png");
}

void NeonObj::Update() {
	// ==========================================
	// 💡 フリッカー（チカチカ）の計算
	// ==========================================
	float currentIntensity = intensity_;

	// ImGuiでフリッカーがONになっている時だけ計算する
	if (isFlicker_) {
		time_ += 1.0f / 60.0f;
		if (std::sin(time_ * 12.0f) > 0.7f) {
			float noise = (rand() % 100) / 100.0f;
			currentIntensity *= (0.2f + noise * 0.8f);
		}
		if (rand() % 1000 < 10) {
			currentIntensity = 0.0f;
		}
	}

	// ==========================================
	// 💡 モデルへ質感パラメーターを送信
	// ==========================================
	// ※注意：NeonModel側に、NeonSignで作ったのと同じ SetMaterial 関数を用意しておく必要があります！
	// もし無ければ、とりあえずモデルが持っている SetNeonColor などを呼んでください。
	model_->SetNeonColor(currentIntensity, color_[0], color_[1], color_[2]);
	// model_->SetMaterial(radius_, softness_, currentIntensity, color_[0], color_[1], color_[2], lengthOffset_);

	// ==========================================
	// 💡 行列の更新（★ここで座標移動が反映される！）
	// ==========================================
	// ImGuiなどで translation_ が書き換えられた後、必ず行列を作り直す！
	transform_.matWorld_ = MakeAffineMatrix(transform_.scale_, transform_.rotation_, transform_.translation_);
	transform_.TransferMatrix();
}

void NeonObj::Draw(const ViewProjection& viewProjection) {
	model_->Draw(transform_, viewProjection, dummyTexture_);
}

void NeonObj::DrawImGui(const std::string& label) {
#ifdef USE_IMGUI
	// labelを使ってウィンドウ名を変える（複数置いた時に混ざらないようにするため）
	ImGui::Begin(label.c_str());

	// ★ posが動かなかったのは、この数値をいじった後に行列を再計算していなかったからです！
	ImGui::DragFloat3("Position", &transform_.translation_.x, 0.1f);
	ImGui::DragFloat3("Scale", &transform_.scale_.x, 0.01f);
	ImGui::DragFloat3("Rotation", &transform_.rotation_.x, 0.05f);

	ImGui::Separator();
	ImGui::ColorEdit3("Color", color_);
	ImGui::SliderFloat("Intensity", &intensity_, 0.0f, 20.0f);
	ImGui::SliderFloat("Radius (太さ)", &radius_, 0.001f, 0.1f);
	ImGui::SliderFloat("Length (長さ)", &lengthOffset_, -1.0f, 1.0f);
	ImGui::SliderFloat("Softness (ぼかし)", &softness_, 0.1f, 50.0f);

	ImGui::Separator();
	// ★ フリッカーのON/OFFスイッチ！
	ImGui::Checkbox("Enable Flicker (チカチカ)", &isFlicker_);

	ImGui::End();
#endif
}