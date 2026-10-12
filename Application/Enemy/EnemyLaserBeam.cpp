#include "EnemyLaserBeam.h"
#include <cmath>
#include "Application/Character/Player.h"
#include "GameScene.h"

#include "GlobalValiables.h" 
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void EnemyLaserBeam::Initialize(Player* player) {
	BaseEnemy::Initialize(player);

	if (model_ == nullptr) {
		model_ = new NeonModel();
		model_->Initialize("Resources/Enemy/Neon", "Laser.obj");
	}
	whiteTexture_ = TextureManager::Load("Resources/white.png");

	transformAim_.Initialize();
	transformAura_.Initialize();
	transformCore_.Initialize();
	isAiming_ = false;
	isFiring_ = false;

	// HP
	GlobalVariables* global = GlobalVariables::GetInstance();
	const std::string groupName = "EnemyLaserBeam";
	global->CreateGroup(groupName);

	global->AddItem(groupName, "MaxHP", 3);
	maxHp_ = global->GetIntValue(groupName, "MaxHP");

	currentHp_ = maxHp_;
	hitTimer_ = 0;
	flashTimer_ = 0;
}

void EnemyLaserBeam::Update() {

	if (hitTimer_ > 0) {
		hitTimer_--;
	}

	if (flashTimer_ > 0) {
		flashTimer_--;
	}

}

void EnemyLaserBeam::SetBeamTransform(const Vector3& start, const Vector3& angles, float length) {
	startPos_ = start;
	
	float scaleX = length * 0.5f;

	Vector3 centerPos = start;
	centerPos.x += std::cos(angles.z) * scaleX;
	centerPos.y += std::sin(angles.z) * scaleX;
	centerPos.z = start.z;

	worldTransform_.translation_ = centerPos;
	worldTransform_.rotation_ = angles;
	worldTransform_.scale_ = { scaleX, 1.0f, 1.0f };
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	// 予測線
	transformAim_.translation_ = centerPos;
	transformAim_.rotation_ = angles;
	transformAim_.scale_ = { scaleX, 0.05f, 0.05f };
	transformAim_.matWorld_ = MakeAffineMatrix(transformAim_.scale_, transformAim_.rotation_, transformAim_.translation_);

	// オーラ：四角い箱にならないよう、厚みを 0.4f まで絞る
	transformAura_.translation_ = centerPos;
	transformAura_.rotation_ = angles;
	transformAura_.scale_ = { scaleX, 0.4f, 0.4f };
	transformAura_.matWorld_ = MakeAffineMatrix(transformAura_.scale_, transformAura_.rotation_, transformAura_.translation_);

	// コア：純白の鋭い芯
	transformCore_.translation_ = centerPos;
	transformCore_.rotation_ = angles;
	transformCore_.scale_ = { scaleX, 0.1f, 0.1f };
	transformCore_.matWorld_ = MakeAffineMatrix(transformCore_.scale_, transformCore_.rotation_, transformCore_.translation_);
}

void EnemyLaserBeam::DrawNeon(const ViewProjection& viewProjection) {

	if (flashTimer_ > 0 && !isAiming_) {
		model_->SetNeonColor(20.0f, 1.0f, 1.0f, 1.0f);
		model_->Draw(transformAura_, viewProjection, whiteTexture_);
		model_->Draw(transformCore_, viewProjection, whiteTexture_);
		return;
	}

	if (isAiming_) {
		model_->SetNeonColor(2.0f, 1.0f, 0.0f, 0.0f);
		model_->Draw(transformAim_, viewProjection, whiteTexture_);
	}
	else if (isFiring_) {
		model_->SetNeonColor(5.0f, 1.0f, 0.05f, 0.0f);
		model_->Draw(transformAura_, viewProjection, whiteTexture_);

		model_->SetNeonColor(15.0f, 1.0f, 0.8f, 0.5f);
		model_->Draw(transformCore_, viewProjection, whiteTexture_);
	}
}

float EnemyLaserBeam::GetDistanceTo(const Vector3& targetPos) {
	// 狙い中の時は当たり判定を無くす！
	if (!isFiring_) return 9999.0f;

	Vector3 center = worldTransform_.translation_;
	Vector3 xAxis = {
		worldTransform_.matWorld_.m[0][0],
		worldTransform_.matWorld_.m[0][1],
		worldTransform_.matWorld_.m[0][2]
	};

	float lengthRatio = 1.0f;
	Vector3 start = { center.x - xAxis.x * lengthRatio, center.y - xAxis.y * lengthRatio, center.z - xAxis.z * lengthRatio };
	Vector3 end = { center.x + xAxis.x * lengthRatio, center.y + xAxis.y * lengthRatio, center.z + xAxis.z * lengthRatio };

	Vector3 v = { end.x - start.x, end.y - start.y, end.z - start.z };
	Vector3 w = { targetPos.x - start.x, targetPos.y - start.y, targetPos.z - start.z };

	float c1 = w.x * v.x + w.y * v.y + w.z * v.z;
	float c2 = v.x * v.x + v.y * v.y + v.z * v.z;
	if (c2 == 0.0f) return 0.0f;

	Vector3 closest;
	if (c1 <= 0.0f) closest = start;
	else if (c2 <= c1) closest = end;
	else {
		float b = c1 / c2;
		closest = { start.x + v.x * b, start.y + v.y * b, start.z + v.z * b };
	}

	Vector3 diff = { targetPos.x - closest.x, targetPos.y - closest.y, targetPos.z - closest.z };
	diff.z *= 0.2f;
	return std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
}

void EnemyLaserBeam::OnCollision() {

	if (player_) {
		float distToPlayer = GetDistanceTo(player_->GetWorldPosition());
		// 自機とレーザーが触れている距離ならスキップ
		if (distToPlayer <= 2.0f) {
			return;
		}
	}

	if (gameScene_ && gameScene_->GetParticleManager()) {
		gameScene_->GetParticleManager()->EmitStar(startPos_, 5, { 1.0f, 1.0f, 0.5f });
	}

	if (hitTimer_ > 0) return;

	currentHp_--;
	hitTimer_ = 4;
	flashTimer_ = 5; // フラッシュ演出

	if (currentHp_ <= 0) {
		isDead_ = true;
	}
}



void EnemyLaserBeam::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::PushID(this);
	ImGui::Text("--- Laser Beam HP Settings ---");

	ImGui::SliderInt("Max HP", &maxHp_, 1, 100);
	ImGui::Text("Current HP: %d", currentHp_);

	ImGui::Separator();
	if (ImGui::Button("SAVE BEAM SETTINGS (JSON)")) {
		GlobalVariables* global = GlobalVariables::GetInstance();
		const std::string groupName = "EnemyLaserBeam";

		global->SetValue(groupName, "MaxHP", maxHp_);
		global->SaveFile(groupName);
	}
	ImGui::PopID();
#endif
}