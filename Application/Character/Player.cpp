#include "Application/Character/Player.h"
#include <cassert>
#include "DirectXCommon.h"
#include <algorithm>
#include <externals/nlohmann/json.hpp>
#include "GlobalValiables.h"
#include "CollisionConfig.h"
#include "BodyModel.h"
#include <cmath>
#include <vector>
#include <cstdlib>


#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

/*---------------------
デストラクタ
------------------------*/
Player::~Player() {
	for (PlayerBullet* bullet : bullets_) {
		delete bullet;

	}

	delete modelCore_;
	delete modelOuterRing_;
	delete modelWingBase_;
	delete modelInnerRing_;
	delete modelWingNeon_;
	delete pixelModel_;
}

/*----------------
初期化
-----------------------*/
void Player::Initialize() {

	dummyTexture_ = TextureManager::Load("Resources/Player/PlayerTex.png");


	// テクスチャ読み込み
	//textureHandle_ = TextureManager::Load("Resources/block.png");


	worldTransform_.Initialize();
	transformRot_.Initialize();
	transformStat_.Initialize();
	// 動かないring
	modelOuterRing_ = new BodyModel();
	modelOuterRing_->Initialize("Resources/Player", "mech_PlayerRing.obj");

	// コア
	modelCore_ = new BodyModel();
	modelCore_->Initialize("Resources/Player", "mech_core.obj");

	// 羽
	modelWingBase_ = new BodyModel();
	modelWingBase_->Initialize("Resources/Player", "mech_wing.obj");

	// --- 光るパーツ ---
	modelInnerRing_ = new NeonModel();
	modelInnerRing_->Initialize("Resources/Player", "mech_core.ring.obj");

	modelWingNeon_ = new NeonModel();
	modelWingNeon_->Initialize("Resources/Player", "mech_Neonwing.obj");



	worldTransform_.scale_ = { 1.0f, 1.0f, 1.0f };
	worldTransform_.rotation_ = { 0.0f, 0.0f, 0.0f };
	worldTransform_.translation_ = { 0.0f, -2.0f, 15.0f };

	// シングルトンインスタンスを取得する
	input_ = Input::GetInstance();


	// デバッガによる確認
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "Player";

	// グループを追加
	GlobalVariables::GetInstance()->CreateGroup(groupName);

	globalVariables->AddItem(groupName, "Test", 90);

	globalVariables->AddItem(groupName, "moveSpeed", kCharacterSpeed);

	/*----------------------------
	弾
	---------------------------------*/
	bulletModel_ = new NeonModel();
	bulletModel_->Initialize("Resources/Bullet", "mech_PlayerBullet.obj");

	// 自分の属性をプレイヤーに設定
	SetCollisionAttribute(kCollisionAttributePlayer);
	// 当たる相手をプレイヤー以外」に設定
	SetCollisionMask(~kCollisionAttributePlayer);

	/*-----------------------------
	死亡演出
	-----------------------------*/
	pixelModel_ = new NeonModel();
	pixelModel_->Initialize("Resources/Enemy/Neon", "Laser.obj");

	whiteTexture_ = TextureManager::Load("Resources/white.png");

	for (int i = 0; i < kMaxPixels; ++i) {
		pixelTransforms_[i].Initialize();
		pixels_[i].isActive = false;
	}


	/*-----------------
	音
	--------------------------*/
	shotSound_ = Audio::GetInstance()->SoundLoadWave("Sounds/Shot.wav");
	rebuildSound_ = Audio::GetInstance()->SoundLoadWave("Sounds/Rebuilding.wav");

	wasAttackedSound_ = Audio::GetInstance()->SoundLoadWave("Sounds/WasAttacked.wav");

}

/*--------------------
調整項目の適用
-------------------------*/
void Player::RegisterGlobalVariables() {}

void Player::ApplyGlobalVariables() {
	// 調整項目の適用
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	const char* groupName = "Player";

	kCharacterSpeed= globalVariables->GetFloatValue(groupName, "moveSpeed");
}

/*----------------------
旋回処理
------------------------------*/
void Player::Rotate() {

// 回転の速さ
	const float kRotaSpeed = 0.02f;

}

/*------------------------
攻撃
----------------------------*/
void Player::Attack() {

	if (input_->TriggerKey(DIK_SPACE)) {

		// 音の再生
		Audio::GetInstance()->SoundPlayWave(shotSound_);
		// 弾の速度 
		const float kBulletSpeed = 1.0f;
		Vector3 velocity(0, 0, kBulletSpeed);

		// 速度ベクトルを自機の向きに合わせて回転させる
		velocity = TransformNormal(velocity, worldTransform_.matWorld_);

		Vector3 spawnPos = GetWorldPosition();


		PlayerBullet* newBullet = nullptr;
		for (PlayerBullet* bullet : bullets_) {
			if (bullet->IsDead()) {
				newBullet = bullet;
				break; 
			}
		}

		if (newBullet == nullptr) {
			newBullet = new PlayerBullet();
			bullets_.push_back(newBullet);
		}

		// 弾を初期化して発射！
		newBullet->Initialize(bulletModel_, spawnPos, velocity, worldTransform_.rotation_);
	}
}

/*-----------------------------
衝突時コールバック
------------------------------*/
void Player::OnCollision() {
	if (isDead_ || invincibleTimer_ > 0) return;
	if (isDead_) return;

	Audio::GetInstance()->SoundPlayWave(wasAttackedSound_);
	isDead_ = true;
	deathTimer_ = 100;
	hasPlayedRebuildSound_ = false;

	if (cameraShake_ != nullptr) {
		cameraShake_->Start(3.5f, 20);
	}



	for (int i = 0; i < kMaxPixels; ++i) {
		pixels_[i].isActive = true;

		pixels_[i].localOffset = {
			(rand() % 100 - 50) / 100.0f * 1.5f,
			(rand() % 100 - 50) / 100.0f * 1.5f,
			(rand() % 100 - 50) / 100.0f * 1.5f
		};


		pixels_[i].position = worldTransform_.translation_;
		pixels_[i].position.x += pixels_[i].localOffset.x;
		pixels_[i].position.y += pixels_[i].localOffset.y;
		pixels_[i].position.z += pixels_[i].localOffset.z;

		float angleX = (rand() % 360) * 3.14159f / 180.0f;
		float angleY = (rand() % 360) * 3.14159f / 180.0f;
		float speed = (rand() % 100 / 100.0f) * 3.0f + 1.0f;

		pixels_[i].velocity.x = std::cos(angleY) * std::sin(angleX) * speed;
		pixels_[i].velocity.y = std::sin(angleY) * speed;
		pixels_[i].velocity.z = std::cos(angleY) * std::cos(angleX) * speed;

		pixels_[i].rotation = { 0, 0, 0 };
		pixels_[i].rotSpeed = {
			(rand() % 100 - 50) / 100.0f * 0.4f,
			(rand() % 100 - 50) / 100.0f * 0.4f,
			(rand() % 100 - 50) / 100.0f * 0.4f
		};
	}
	isDead_ = true;
}
/*-------------------------
更新処理
----------------------------*/
void Player::Update(const Matrix4x4& parentMatrix) {

	// 機能の調整
	ApplyGlobalVariables();
	if (isDead_) {
		deathTimer_--;

		

		if (deathTimer_ > 50) {
			for (int i = 0; i < kMaxPixels; ++i) {
				if (!pixels_[i].isActive) continue;
				pixels_[i].position.x += pixels_[i].velocity.x;
				pixels_[i].position.y += pixels_[i].velocity.y;
				pixels_[i].position.z += pixels_[i].velocity.z;

				pixels_[i].velocity.x *= 0.92f;
				pixels_[i].velocity.y *= 0.92f;
				pixels_[i].velocity.z *= 0.92f;

				pixels_[i].rotation.x += pixels_[i].rotSpeed.x;
				pixels_[i].rotation.y += pixels_[i].rotSpeed.y;
				pixels_[i].rotation.z += pixels_[i].rotSpeed.z;
			}
		}
		else {

			if (!hasPlayedRebuildSound_) {
				Audio::GetInstance()->SoundPlayWave(rebuildSound_);
				hasPlayedRebuildSound_ = true; 
			}

			Vector3 center = worldTransform_.translation_;
			for (int i = 0; i < kMaxPixels; ++i) {
				if (!pixels_[i].isActive) continue;

				Vector3 targetPos = {
					center.x + pixels_[i].localOffset.x,
					center.y + pixels_[i].localOffset.y,
					center.z + pixels_[i].localOffset.z
				};

				pixels_[i].position.x += (targetPos.x - pixels_[i].position.x) * 0.1f;
				pixels_[i].position.y += (targetPos.y - pixels_[i].position.y) * 0.1f;
				pixels_[i].position.z += (targetPos.z - pixels_[i].position.z) * 0.1f;

				pixels_[i].rotation.x += (0.0f - pixels_[i].rotation.x) * 0.1f;
				pixels_[i].rotation.y += (0.0f - pixels_[i].rotation.y) * 0.1f;
				pixels_[i].rotation.z += (0.0f - pixels_[i].rotation.z) * 0.1f;
			}
		}

		// GPUの箱へ転送
		for (int i = 0; i < kMaxPixels; ++i) {
			if (!pixels_[i].isActive) continue;
			pixelTransforms_[i].translation_ = pixels_[i].position;
			pixelTransforms_[i].rotation_ = pixels_[i].rotation;
			pixelTransforms_[i].scale_ = { 0.15f, 0.15f, 0.15f };

			Matrix4x4 pixLocal = MakeAffineMatrix(pixelTransforms_[i].scale_, pixelTransforms_[i].rotation_, pixelTransforms_[i].translation_);
			pixelTransforms_[i].matWorld_ = Multiply(pixLocal, parentMatrix);
			pixelTransforms_[i].TransferMatrix();
		}

		if (deathTimer_ <= 0) {
			isDead_ = false;
			for (int i = 0; i < kMaxPixels; ++i) pixels_[i].isActive = false;
		}

		invincibleTimer_ = 180;
	}
	else {

		if (invincibleTimer_ > 0) {
			invincibleTimer_--;
		}

		//　旋回処理
		Rotate();


		/*------------------------------
		弾
		-------------------------------*/


		/*-------------------------------
		キャラクター移動処理
		------------------------------*/

		// キャラクターの移動ベクトル
		Vector3 move = { 0,0,0 };


		// 押した方向へ移動ベクトルを変更(左右)
		if (input_->PushKey(DIK_LEFT)) {
			move.x -= kCharacterSpeed;
		}
		else if (input_->PushKey(DIK_RIGHT)) {
			move.x += kCharacterSpeed;
		}

		// 押した方向へ移動ベクトル(上下)
		if (input_->PushKey(DIK_UP)) {
			move.y += kCharacterSpeed;
		}
		else if (input_->PushKey(DIK_DOWN)) {
			move.y -= kCharacterSpeed;
		}

		float targetRoll = 0.0f;

		// 押した方向へ移動ベクトルを変更
		if (input_->PushKey(DIK_LEFT)) {
			move.x -= kCharacterSpeed;
			targetRoll = 0.5f;  // 左移動中は左に傾ける
		}
		else if (input_->PushKey(DIK_RIGHT)) {
			move.x += kCharacterSpeed;
			targetRoll = -0.5f; // 右移動中は右に傾ける
		}

		// 現在の傾きから目標の傾きへ、滑らかに近づける
		worldTransform_.rotation_.z += (targetRoll - worldTransform_.rotation_.z) * 0.1f;

		// 座標移動
		worldTransform_.translation_.x += move.x;
		worldTransform_.translation_.y += move.y;
		worldTransform_.translation_.z += move.z;

		// 移動限界座標
		const float kMoveLimitX = 12.0f;
		const float kMoveLimitY = 8.0f;

		// 範囲を超えない処理
		worldTransform_.translation_.x = (std::max)(worldTransform_.translation_.x, -kMoveLimitX);
		worldTransform_.translation_.x = (std::min)(worldTransform_.translation_.x, kMoveLimitX);
		worldTransform_.translation_.y = (std::max)(worldTransform_.translation_.y, -kMoveLimitY);
		worldTransform_.translation_.y = (std::min)(worldTransform_.translation_.y, kMoveLimitY);


		Matrix4x4 localMatrix = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
		worldTransform_.matWorld_ = Multiply(localMatrix, parentMatrix);
		worldTransform_.TransferMatrix();

		// 攻撃処理
		Attack();
	}

	// 弾更新
	for (PlayerBullet* bullet : bullets_) {
		if (enemies_) {
			bullet->Update(*enemies_); // 敵情報を渡す！
		}
		else {
			std::list<BaseEnemy*> empty;
			bullet->Update(empty);
		}
	}





	// ==========================================
	//  回らないグループ
	// ==========================================
	transformStat_.translation_ = worldTransform_.translation_;
	transformStat_.rotation_ = worldTransform_.rotation_;
	transformStat_.scale_ = worldTransform_.scale_;
	// 行列を計算して転送
	Matrix4x4 statLocal = MakeAffineMatrix(transformStat_.scale_, transformStat_.rotation_, transformStat_.translation_);
	transformStat_.matWorld_ = Multiply(statLocal, parentMatrix);
	transformStat_.TransferMatrix();

	// ==========================================
	// 回るグループ
	// ==========================================
	coreSpinAngle_ += 0.05f; // くるくる回すスピード


	float colorSpeed = 0.5f; // 色が変化するスピード
	neonColor_[0] = std::sin(coreSpinAngle_ * colorSpeed) * 0.5f + 0.5f;               // R (赤)
	neonColor_[1] = std::sin(coreSpinAngle_ * colorSpeed + 2.094395f) * 0.5f + 0.5f; // G (緑)
	neonColor_[2] = std::sin(coreSpinAngle_ * colorSpeed + 4.188790f) * 0.5f + 0.5f; // B (青)

	transformRot_.translation_ = worldTransform_.translation_;
	transformRot_.rotation_ = {
			worldTransform_.rotation_.x + coreSpinAngle_ * 0.8f,
			worldTransform_.rotation_.y + coreSpinAngle_ * 1.3f,
			worldTransform_.rotation_.z + coreSpinAngle_ * 1.0f
	};

	transformRot_.scale_ = worldTransform_.scale_;
	// 行列を計算して転送
	Matrix4x4 rotLocal = MakeAffineMatrix(transformRot_.scale_, transformRot_.rotation_, transformRot_.translation_);
	transformRot_.matWorld_ = Multiply(rotLocal, parentMatrix);
	transformRot_.TransferMatrix();

}



/*--------------------------
描画処理
--------------------*/
void Player::Draw(const ViewProjection& viewProjection) {

	if (isDead_) return;

	if (invincibleTimer_ > 0 && (invincibleTimer_ % 4 >= 2)) return;

	modelCore_->SetColor(bodyColor_[0], bodyColor_[1], bodyColor_[2], 1.0f);
	modelOuterRing_->SetColor(bodyColor_[0], bodyColor_[1], bodyColor_[2], 1.0f);
	modelWingBase_->SetColor(bodyColor_[0], bodyColor_[1], bodyColor_[2], 1.0f);

	Vector3 lightDir = { -1.0f, -1.0f, 1.0f }; // 左上・手前からの固定ライト


	float bodyLightIntensity = 1.5f;

	// 光の色もネオンの色ではなく、純粋な白（1,1,1）で照らして、ボディ本来の紫を活かす
	modelCore_->SetLight(bodyLightIntensity, 1.0f, 1.0f, 1.0f, lightDir);
	modelOuterRing_->SetLight(bodyLightIntensity, 1.0f, 1.0f, 1.0f, lightDir);
	modelWingBase_->SetLight(bodyLightIntensity, 1.0f, 1.0f, 1.0f, lightDir);

	// --- 暗いパーツを描画 ---
	modelCore_->Draw(transformRot_, viewProjection, dummyTexture_);
	modelOuterRing_->Draw(transformStat_, viewProjection, dummyTexture_);
	modelWingBase_->Draw(transformStat_, viewProjection, dummyTexture_);


}

void Player::DrawNeon(const ViewProjection& viewProjection) {
	if (isDead_) {


		if (pixelModel_ != nullptr) {
			
			pixelModel_->SetNeonColor(20.0f, neonColor_[0], neonColor_[1], neonColor_[2]);
			for (int i = 0; i < kMaxPixels; ++i) {
				if (pixels_[i].isActive) {
					pixelModel_->Draw(pixelTransforms_[i], viewProjection, whiteTexture_);
				}
			}
		}
	}
	else {
		if (invincibleTimer_ == 0 || (invincibleTimer_ % 4 < 2)) {
			if (modelInnerRing_ != nullptr) {
				modelInnerRing_->SetNeonColor(neonIntensity_, neonColor_[0], neonColor_[1], neonColor_[2]);
				modelInnerRing_->Draw(transformRot_, viewProjection, dummyTexture_);
			}
			if (modelWingNeon_ != nullptr) {
				modelWingNeon_->SetNeonColor(neonIntensity_, neonColor_[0], neonColor_[1], neonColor_[2]);
				modelWingNeon_->Draw(transformStat_, viewProjection, dummyTexture_);
			}
		}
	}
	for (PlayerBullet* bullet : bullets_) {
		bullet->Draw(viewProjection);
	}
}


Vector3  Player::GetWorldPosition() {
	Vector3 worldPos;
	// ワールド座標の平行移動成分を取得
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];

	return worldPos;
}
void Player::SetPointLight(const Vector3& pos, const Vector3& color, float intensity, float radius, const Vector3& cameraPos) {
	if (modelCore_) modelCore_->SetPointLight(pos, color, intensity, radius, cameraPos);
	if (modelOuterRing_) modelOuterRing_->SetPointLight(pos, color, intensity, radius, cameraPos);
	if (modelWingBase_) modelWingBase_->SetPointLight(pos, color, intensity, radius, cameraPos);
}



void Player::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::Text("Position: X: %f, Y: %f, Z: %f",
		worldTransform_.translation_.x, worldTransform_.translation_.y, worldTransform_.translation_.z);

	ImGui::Separator();
	ImGui::Text("--- Neon Settings ---");
	ImGui::ColorEdit3("Neon Color", neonColor_);
	ImGui::SliderFloat("Neon Intensity", &neonIntensity_, 0.1f, 20.0f);


	ImGui::Separator();
	ImGui::Text("--- Body Settings ---");
	ImGui::ColorEdit3("Body Color", bodyColor_);
#endif
}

void Player::DrawUI(const ViewProjection& viewProjection) {
	std::vector<BaseEnemy*> drawnTargets;

	for (PlayerBullet* bullet : bullets_) {
		if (bullet->IsDead()) continue;

		BaseEnemy* target = bullet->GetTarget();

		// ターゲットがいる場合
		if (target != nullptr) {
			// drawnTargetsリストの中に、同じ敵がすでにいるか探す
			auto it = std::find(drawnTargets.begin(), drawnTargets.end(), target);

			// まだリストにいないなら描画！
			if (it == drawnTargets.end()) {
				bullet->DrawUI(viewProjection);
				drawnTargets.push_back(target); // 描画済みとしてリストに記録する
			}
		}
	}
}