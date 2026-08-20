#include "WeakEnemySpinCore.h"
#include "GlobalValiables.h" 
#include "EnemyStateHold.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

NeonModel* WeakEnemySpinCore::sNeonFrame = nullptr;
NeonModel* WeakEnemySpinCore::sNeonCore = nullptr;

// マスターカラーの初期値
Vector3 WeakEnemySpinCore::sSpinCoreColor = { 0.0f, 0.8f, 1.0f };
float WeakEnemySpinCore::sSpinCoreIntensity = 10.0f;

void WeakEnemySpinCore::StaticInitialize() {
	if (sNeonFrame == nullptr) {
		sNeonFrame = new NeonModel();
		sNeonFrame->Initialize("Resources/Enemy/Neon", "weakEnemyDiaFrame.obj");

		sNeonCore = new NeonModel();
		sNeonCore->Initialize("Resources/Enemy/Neon", "weakEnemyDiaCore.obj");
	}
}

void WeakEnemySpinCore::Initialize(Player* player) {
	// 親の初期化
	BaseEnemy::Initialize(player);

	ClearTimedCalls();

	modelBase_ = nullptr;
	modelLines_ = nullptr;
	modelRing_ = nullptr;
	for (int i = 0; i < 5; ++i) {
		modelTails_[i] = nullptr;
	}

	// コア用のTransform初期化
	transformCore_.Initialize();

	// 色情報の読み込み
	ApplyGlobalVariables();

	ChangeState(new EnemyStateHold());
}

void WeakEnemySpinCore::ApplyGlobalVariables() {
	GlobalVariables* global = GlobalVariables::GetInstance();
	const std::string groupName = "WeakEnemySpinCore";
	global->CreateGroup(groupName);

	global->AddItem(groupName, "NeonColor", Vector3(0.0f, 0.8f, 1.0f));
	global->AddItem(groupName, "NeonIntensity", 10.0f);

	sSpinCoreColor = global->GetVector3Value(groupName, "NeonColor");
	sSpinCoreIntensity = global->GetFloatValue(groupName, "NeonIntensity");
}

void WeakEnemySpinCore::Update() {
	BaseEnemy::Update();

	transformCore_.translation_ = worldTransform_.translation_;
	transformCore_.scale_ = worldTransform_.scale_;

	// コアだけを回転させる
	transformCore_.rotation_.y += 0.05f;
	transformCore_.rotation_.z += 0.03f;

	// コア専用の行列を更新
	transformCore_.matWorld_ = MakeAffineMatrix(transformCore_.scale_, transformCore_.rotation_, transformCore_.translation_);
	transformCore_.TransferMatrix();
}

void WeakEnemySpinCore::DrawNeon(const ViewProjection& viewProjection) {
	// 外枠の描画
	if (sNeonFrame) {
		sNeonFrame->SetNeonColor(sSpinCoreIntensity, sSpinCoreColor.x, sSpinCoreColor.y, sSpinCoreColor.z);
		sNeonFrame->Draw(worldTransform_, viewProjection, dummyTexture_);
	}

	// 中のコアの描画
	if (sNeonCore) {
		sNeonCore->SetNeonColor(sSpinCoreIntensity, sSpinCoreColor.x, sSpinCoreColor.y, sSpinCoreColor.z);
		sNeonCore->Draw(transformCore_, viewProjection, dummyTexture_);
	}
}

void WeakEnemySpinCore::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::PushID(this);

	ImGui::DragFloat3("SpinCore Pos", &worldTransform_.translation_.x, 0.1f);
	ImGui::Separator();
	ImGui::Text("SpinCore Enemy Master Settings");

	float nColor[3] = { sSpinCoreColor.x, sSpinCoreColor.y, sSpinCoreColor.z };
	if (ImGui::ColorEdit3("Neon Color", nColor)) {
		sSpinCoreColor = { nColor[0], nColor[1], nColor[2] };
	}

	ImGui::SliderFloat("Neon Intensity", &sSpinCoreIntensity, 0.0f, 20.0f);
	ImGui::Separator();

	if (ImGui::Button("SAVE SPINCORE SETTINGS (JSON)")) {
		GlobalVariables* global = GlobalVariables::GetInstance();
		const std::string groupName = "WeakEnemySpinCore";
		global->SetValue(groupName, "NeonColor", sSpinCoreColor);
		global->SetValue(groupName, "NeonIntensity", sSpinCoreIntensity);
		global->SaveFile(groupName);
	}

	ImGui::PopID();
#endif
}