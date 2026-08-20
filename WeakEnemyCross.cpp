#include "WeakEnemyCross.h"
#include "GlobalValiables.h"
#include "EnemyStateHold.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

// 静的変数の実体定義
BodyModel* WeakEnemyCross::sBodyModelCross = nullptr;
NeonModel* WeakEnemyCross::sNeonModelCross = nullptr;
NeonModel* WeakEnemyCross::sNeonPatternCross = nullptr; 

void WeakEnemyCross::StaticInitialize() {
	if (sBodyModelCross == nullptr) {
		sBodyModelCross = new BodyModel();
		// 暗いボディの読み込み
		sBodyModelCross->Initialize("Resources/Enemy", "weakEnemyCross.obj");

		sNeonModelCross = new NeonModel();
		// 光るネオンの読み込み
		sNeonModelCross->Initialize("Resources/Enemy/Neon", "weakEnemyBodyCross.obj");

		// 光るネオンの読み込み
		sNeonPatternCross = new NeonModel();
		sNeonPatternCross->Initialize("Resources/Enemy/Neon", "weakEnemyCross.obj");
	}
}

void WeakEnemyCross::Initialize(Player* player) {
	// 親の初期化
	BaseEnemy::Initialize(player);

	ClearTimedCalls();

	// モデルのセット
	modelBase_ = sBodyModelCross;   // 暗いボディ
	modelLines_ = sNeonModelCross;  // 光るネオン
	modelPattern_ = sNeonPatternCross; 

	modelRing_ = nullptr;
	for (int i = 0; i < 5; ++i) {
		modelTails_[i] = nullptr;
	}

	ApplyGlobalVariables();
	ChangeState(new EnemyStateHold());
}

/*-------------------------
jsonデータの読み込み
------------------------*/
void WeakEnemyCross::ApplyGlobalVariables() {
	GlobalVariables* global = GlobalVariables::GetInstance();
	const std::string groupName = "WeakEnemyCross";

	// グループを作成
	global->CreateGroup(groupName);

	// デフォルト値を登録
	global->AddItem(groupName, "BodyColor", Vector3(0.3f, 0.0f, 0.0f));
	global->AddItem(groupName, "NeonColor", Vector3(1.0f, 0.2f, 0.2f));
	global->AddItem(groupName, "NeonIntensity", 10.0f);

	// JSONから最新の値を取得して、メンバー変数に入れる
	Vector3 bColor = global->GetVector3Value(groupName, "BodyColor");
	bodyColor_[0] = bColor.x;
	bodyColor_[1] = bColor.y;
	bodyColor_[2] = bColor.z;

	Vector3 nColor = global->GetVector3Value(groupName, "NeonColor");
	neonColor_[0] = nColor.x;
	neonColor_[1] = nColor.y;
	neonColor_[2] = nColor.z;

	neonIntensity_ = global->GetFloatValue(groupName, "NeonIntensity");
}

/*------------------
ImGui
--------------------------*/
void WeakEnemyCross::DrawImGui() {
#ifdef USE_IMGUI
	// 座標の操作
	ImGui::PushID(this);

	ImGui::DragFloat3("Cross Pos", &worldTransform_.translation_.x, 0.1f);
	ImGui::Separator();

	ImGui::Text("Cross Enemy Visual Settings");

	// 色と輝度のスライダー
	ImGui::ColorEdit3("Body Color", bodyColor_);
	ImGui::ColorEdit3("Neon Color", neonColor_);
	ImGui::SliderFloat("Neon Intensity", &neonIntensity_, 0.0f, 20.0f);

	ImGui::Separator();

	if (ImGui::Button("SAVE CROSS SETTINGS (JSON)")) {
		GlobalVariables* global = GlobalVariables::GetInstance();
		const std::string groupName = "WeakEnemyCross";

		global->SetValue(groupName, "BodyColor", Vector3(bodyColor_[0], bodyColor_[1], bodyColor_[2]));
		global->SetValue(groupName, "NeonColor", Vector3(neonColor_[0], neonColor_[1], neonColor_[2]));
		global->SetValue(groupName, "NeonIntensity", neonIntensity_);

		global->SaveFile(groupName);
	}

	ImGui::PopID();
#endif
}

// 描画処理
void WeakEnemyCross::DrawNeon(const ViewProjection& viewProjection) {
	if (modelBase_) {
		modelBase_->SetColor(bodyColor_[0], bodyColor_[1], bodyColor_[2], 1.0f);
		Vector3 lightDir = { -1.0f, -1.0f, 1.0f };
		modelBase_->SetLight(1.0f, 1.0f, 1.0f, 1.0f, lightDir);

		modelBase_->Draw(worldTransform_, viewProjection, dummyTexture_);
	}

	// 光るネオン（枠）の描画
	if (modelLines_) {
		modelLines_->SetNeonColor(neonIntensity_, neonColor_[0], neonColor_[1], neonColor_[2]);
		modelLines_->Draw(worldTransform_, viewProjection, dummyTexture_);
	}

	// 光るネオン（模様）の描画
	if (modelPattern_) {
		modelPattern_->SetNeonColor(neonIntensity_, neonColor_[0], neonColor_[1], neonColor_[2]);
		modelPattern_->Draw(worldTransform_, viewProjection, dummyTexture_);
	}
}