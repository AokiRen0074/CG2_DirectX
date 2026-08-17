#include "WeakEnemyTriangle.h"
#include "GlobalValiables.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

NeonModel* WeakEnemyTriangle::sNeonTriangle = nullptr;

// マスターカラーの初期値
Vector3 WeakEnemyTriangle::sTriangleColor = { 0.8f, 1.0f, 0.0f };
float WeakEnemyTriangle::sTriangleIntensity = 10.0f;

void WeakEnemyTriangle::StaticInitialize() {
	if (sNeonTriangle == nullptr) {
		sNeonTriangle = new NeonModel();
		sNeonTriangle->Initialize("Resources/Enemy/Neon", "weakEnemyTri.obj");
	}
}

void WeakEnemyTriangle::Initialize(Player* player) {
	BaseEnemy::Initialize(player);

	modelBase_ = nullptr;
	modelLines_ = nullptr;
	modelRing_ = nullptr;
	for (int i = 0; i < 5; ++i) {
		modelTails_[i] = nullptr;
	}

	ApplyGlobalVariables();
}

void WeakEnemyTriangle::ApplyGlobalVariables() {
	GlobalVariables* global = GlobalVariables::GetInstance();
	const std::string groupName = "WeakEnemyTriangle";
	global->CreateGroup(groupName);

	global->AddItem(groupName, "NeonColor", Vector3(0.8f, 1.0f, 0.0f));
	global->AddItem(groupName, "NeonIntensity", 10.0f);

	sTriangleColor = global->GetVector3Value(groupName, "NeonColor");
	sTriangleIntensity = global->GetFloatValue(groupName, "NeonIntensity");
}

void WeakEnemyTriangle::DrawNeon(const ViewProjection& viewProjection) {
	if (sNeonTriangle) {
		sNeonTriangle->SetNeonColor(sTriangleIntensity, sTriangleColor.x, sTriangleColor.y, sTriangleColor.z);
		sNeonTriangle->Draw(worldTransform_, viewProjection, dummyTexture_);
	}
}

void WeakEnemyTriangle::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::PushID(this);

	ImGui::DragFloat3("Triangle Pos", &worldTransform_.translation_.x, 0.1f);
	ImGui::Separator();
	ImGui::Text("Triangle Enemy Master Settings");

	float nColor[3] = { sTriangleColor.x, sTriangleColor.y, sTriangleColor.z };
	if (ImGui::ColorEdit3("Neon Color", nColor)) {
		sTriangleColor = { nColor[0], nColor[1], nColor[2] };
	}

	ImGui::SliderFloat("Neon Intensity", &sTriangleIntensity, 0.0f, 20.0f);
	ImGui::Separator();

	if (ImGui::Button("SAVE TRIANGLE SETTINGS (JSON)")) {
		GlobalVariables* global = GlobalVariables::GetInstance();
		const std::string groupName = "WeakEnemyTriangle";
		global->SetValue(groupName, "NeonColor", sTriangleColor);
		global->SetValue(groupName, "NeonIntensity", sTriangleIntensity);
		global->SaveFile(groupName);
	}

	ImGui::PopID();
#endif
}