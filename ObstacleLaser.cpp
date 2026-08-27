#include "ObstacleLaser.h"

#include "GlobalValiables.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void ObstacleLaser::Initialize(Player* player) {
	BaseEnemy::Initialize(player);
	isObstacle_ = true;

	if (model_ == nullptr) {
		model_ = new NeonModel();
		model_->Initialize("Resources/Enemy/Neon", "Laser.obj");
	}

	// 真っ白な画像
	whiteTexture_ = TextureManager::Load("Resources/white.png");

	// レーザーの大きさ
	worldTransform_.scale_ = { 15.0f, 1.0f, 1.0f };
	GlobalVariables* global = GlobalVariables::GetInstance();
	const std::string groupName = "ObstacleLaser";
	global->CreateGroup(groupName);

	// 初期値を登録
	global->AddItem(groupName, "NeonColor", Vector3(1.0f, 0.0f, 0.0f));
	global->AddItem(groupName, "NeonIntensity", 5.0f);

	// 登録された数値を読み込んで変数にセット
	Vector3 color = global->GetVector3Value(groupName, "NeonColor");
	neonColor_[0] = color.x;
	neonColor_[1] = color.y;
	neonColor_[2] = color.z;
	neonIntensity_ = global->GetFloatValue(groupName, "NeonIntensity");
	SetRadius(1.0f);

	ClearTimedCalls();
}


void ObstacleLaser::Update() {
	Move({ moveDirection_.x * moveSpeed_, moveDirection_.y * moveSpeed_, moveDirection_.z * moveSpeed_ });

	if (player_) {
		// レーザーのZ座標が、プレイヤーのZ座標より 30.0f 後ろに行ったら
		if (worldTransform_.translation_.z < player_->GetWorldPosition().z - 30.0f) {
			isDead_ = true;
		}
	}

	// 行列の更新
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void ObstacleLaser::OnCollision() {

}

void ObstacleLaser::DrawNeon(const ViewProjection& viewProjection) {
	if (model_) {
		model_->SetNeonColor(neonIntensity_, neonColor_[0], neonColor_[1], neonColor_[2]);
		model_->Draw(worldTransform_, viewProjection, whiteTexture_);
	}
}


/*----------------------
レーザーとターゲットの最短距離の計算
^--------------------------------*/
float ObstacleLaser::GetDistanceTo(const Vector3& targetPos) {
	Vector3 center = GetWorldPosition();

	// ワールド行列から「ローカルのX軸を抜き出す
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
	if (c1 <= 0.0f) {
		closest = start;
	}
	else if (c2 <= c1) {
		closest = end;
	}
	else {
		float b = c1 / c2;
		closest = { start.x + v.x * b, start.y + v.y * b, start.z + v.z * b };
	}

	Vector3 diff = { targetPos.x - closest.x, targetPos.y - closest.y, targetPos.z - closest.z };
	diff.z *= 0.2f; // Z方向の距離を1/5に圧縮

	return std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
}


void ObstacleLaser::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::PushID(this);

	ImGui::Text("--- Laser Color Settings ---");

	// 色と輝度のスライダー
	ImGui::ColorEdit3("Laser Color", neonColor_);
	ImGui::SliderFloat("Laser Intensity", &neonIntensity_, 0.0f, 20.0f);

	ImGui::Separator();

	if (ImGui::Button("SAVE LASER SETTINGS (JSON)")) {
		GlobalVariables* global = GlobalVariables::GetInstance();
		const std::string groupName = "ObstacleLaser";

		// 変更された数値をセットして保存
		global->SetValue(groupName, "NeonColor", Vector3(neonColor_[0], neonColor_[1], neonColor_[2]));
		global->SetValue(groupName, "NeonIntensity", neonIntensity_);
		global->SaveFile(groupName);
	}

	ImGui::PopID();
#endif
}