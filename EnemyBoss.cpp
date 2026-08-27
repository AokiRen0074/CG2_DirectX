#include "EnemyBoss.h"
#include "Application/Character/Player.h"
#include "GameScene.h"
#include "EnemyStateHold.h"
#include <cmath>
#include <algorithm>
#include "GlobalValiables.h" 

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif


EnemyBoss::~EnemyBoss() {
	delete bulletModel_;
	delete laserModel_;
	delete pixelModel_;
}

void EnemyBoss::Initialize(Player* player) {
	BaseEnemy::Initialize(player);
	ClearTimedCalls();

	whiteTexture_ = TextureManager::Load("Resources/white.png");

	armorModels_[0] = new BodyModel();
	armorModels_[0]->Initialize("Resources/Enemy", "Boss_Armor_L.obj");
	armorModels_[1] = new BodyModel();
	armorModels_[1]->Initialize("Resources/Enemy", "Boss_Armor_R.obj");

	for (int i = 0; i < 8; ++i) {
		turretModels_[i] = new BodyModel();
		std::string fileName = "Boss_Turret_" + std::to_string(i + 1) + ".obj";
		turretModels_[i]->Initialize("Resources/Enemy", fileName);
		transformTurrets_[i].Initialize();
		isTurretActive_[i] = true;
	}

	transformArmor_[0].Initialize();
	transformArmor_[1].Initialize();
	isArmorActive_[0] = true;
	isArmorActive_[1] = true;

	coreModel_ = new NeonModel();
	coreModel_->Initialize("Resources/Enemy/Neon", "Boss_Core.obj");

	haloModel_ = new NeonModel();
	haloModel_->Initialize("Resources/Enemy/Neon", "Boss_Halo.obj");

	std::string wingNames[6] = {
		"Boss_Wing_L1.obj", "Boss_Wing_L2.obj", "Boss_Wing_L3.obj",
		"Boss_Wing_R1.obj", "Boss_Wing_R2.obj", "Boss_Wing_R3.obj"
	};
	for (int i = 0; i < 6; ++i) {
		wingModels_[i] = new NeonModel();
		wingModels_[i]->Initialize("Resources/Enemy/Neon", wingNames[i]);
		transformWings_[i].Initialize();
	}


	// レーザーのモデル
	laserModel_ = new NeonModel();
	laserModel_->Initialize("Resources/Enemy/Neon", "Laser.obj");
	transformLaser_.Initialize();
	isLaserActive_ = false;

	pixelModel_ = new NeonModel();
	pixelModel_->Initialize("Resources/Enemy/Neon", "Laser.obj"); // キューブ代わり
	for (int i = 0; i < kMaxChargePixels; ++i) {
		pixelTransforms_[i].Initialize();
		chargePixels_[i].isActive = false;
	}

	transformCore_.Initialize();
	transformHalo_.Initialize();

	GlobalVariables* global = GlobalVariables::GetInstance();
	const std::string groupName = "BossSeraphSettings";
	global->CreateGroup(groupName);

	global->AddItem(groupName, "MaxHP", 500);
	global->AddItem(groupName, "ArmorColor", Vector3(0.4f, 0.4f, 0.45f));
	global->AddItem(groupName, "TurretColor", Vector3(0.3f, 0.3f, 0.35f));
	global->AddItem(groupName, "CoreColor", Vector3(1.0f, 0.0f, 0.2f));
	global->AddItem(groupName, "CoreIntensity", 15.0f);
	global->AddItem(groupName, "WingColor", Vector3(0.0f, 0.8f, 1.0f));
	global->AddItem(groupName, "WingIntensity", 8.0f);
	global->AddItem(groupName, "HaloColor", Vector3(1.0f, 1.0f, 0.8f));
	global->AddItem(groupName, "HaloIntensity", 10.0f);

	maxHp_ = global->GetIntValue(groupName, "MaxHP");
	currentHp_ = maxHp_;

	Vector3 ac = global->GetVector3Value(groupName, "ArmorColor");
	armorColor_[0] = ac.x; armorColor_[1] = ac.y; armorColor_[2] = ac.z;

	Vector3 tc = global->GetVector3Value(groupName, "TurretColor");
	turretColor_[0] = tc.x; turretColor_[1] = tc.y; turretColor_[2] = tc.z;

	Vector3 cc = global->GetVector3Value(groupName, "CoreColor");
	coreColor_[0] = cc.x; coreColor_[1] = cc.y; coreColor_[2] = cc.z;
	coreIntensity_ = global->GetFloatValue(groupName, "CoreIntensity");

	Vector3 wc = global->GetVector3Value(groupName, "WingColor");
	wingColor_[0] = wc.x; wingColor_[1] = wc.y; wingColor_[2] = wc.z;
	wingIntensity_ = global->GetFloatValue(groupName, "WingIntensity");

	Vector3 hc = global->GetVector3Value(groupName, "HaloColor");
	haloColor_[0] = hc.x; haloColor_[1] = hc.y; haloColor_[2] = hc.z;
	haloIntensity_ = global->GetFloatValue(groupName, "HaloIntensity");

	hitTimer_ = 0;
	flashTimer_ = 0;
	animeTime_ = 0.0f;

	currentState_ = BossState::Intro;
	introTimer_ = 0.0f;
	hitTimer_ = 9999;

	// 音
	chargeSound_ = Audio::GetInstance()->SoundLoadWave("Sounds/BossCharge.wav");
	beamSound_ = Audio::GetInstance()->SoundLoadWave("Sounds/Beam.wav");
	smallExplosionSound_ = Audio::GetInstance()->SoundLoadWave("Sounds/SoExplosion.wav");
	bigExplosionSound_ = Audio::GetInstance()->SoundLoadWave("Sounds/BigExplosion.wav");
	hasPlayedChargeSound_ = false;
	hasPlayedBeamSound_ = false;

	for (int i = 0; i < 8; ++i) {
		shockwaveRings_[i].transform.Initialize();
		shockwaveRings_[i].isActive = false;
	}


	ChangeState(new EnemyStateHold());
	WarningUI::GetInstance()->StartWarning();

	worldTransform_.translation_.z = 200.0f;
}

void EnemyBoss::Update() {
	BaseEnemy::Update();


	animeTime_ += 0.05f;

	switch (currentState_) {
	case BossState::Intro: {
		introTimer_ += 1.0f / 60.0f;
		if (introTimer_ >= 3.0f) {
			currentState_ = BossState::Phase1;
			hitTimer_ = 0;
		}


		break;
	}

	case BossState::Phase1: {
		if (hitTimer_ > 0) hitTimer_--;
		if (flashTimer_ > 0) flashTimer_--;

		if (player_) {
			float playerX = player_->GetWorldPosition().x;
			worldTransform_.translation_.x += (playerX - worldTransform_.translation_.x) * 0.02f;
		}


		if (attackPhase_ == 0) {
			AttackTurret();
		}
		else if (attackPhase_ == 1) {
			attackTimer_ += 1.0f / 60.0f;
			if (attackTimer_ >= 3.0f) {
				attackPhase_ = 2;
				attackTimer_ = 0.0f;
			}
		}
		else if (attackPhase_ == 2) {
			AttackLaser();
		}
		else if (attackPhase_ == 3) {
			attackTimer_ += 1.0f / 60.0f;
			if (attackTimer_ >= 3.0f) {
				attackPhase_ = 4; // リング攻撃へ！
				attackTimer_ = 0.0f;
			}
		}
		else if (attackPhase_ == 4) {
			AttackRing(); // リング衝撃波！
		}
		else if (attackPhase_ == 5) {
			attackTimer_ += 1.0f / 60.0f;
			if (attackTimer_ >= 3.0f) {
				attackPhase_ = 0; // 最初に戻る
				attackTimer_ = 0.0f;
			}
		}


		for (int i = 0; i < 8; ++i) {
			if (shockwaveRings_[i].isActive) {

	
				shockwaveRings_[i].transform.translation_.y -= 1.0f;

				shockwaveRings_[i].transform.matWorld_ = MakeAffineMatrix(shockwaveRings_[i].transform.scale_, shockwaveRings_[i].transform.rotation_, shockwaveRings_[i].transform.translation_);
				shockwaveRings_[i].transform.TransferMatrix();

				// 地面の下まで落ち切ったら消す
				if (shockwaveRings_[i].transform.translation_.y < -30.0f) {
					shockwaveRings_[i].isActive = false;
				}

				if (player_ && CheckRingCollision(player_->GetWorldPosition(), shockwaveRings_[i])) {
					player_->OnCollision();
				}
			}
		}
		break;
	}

	case BossState::Phase2: {
		if (hitTimer_ > 0) hitTimer_--;
		if (flashTimer_ > 0) flashTimer_--;
		break;
	}

	case BossState::Dying: {
		int currentFrame = static_cast<int>(animeTime_ * 60.0f);


		int calcVal = 30 - static_cast<int>(animeTime_ * 2.0f);
		int explodeInterval = (calcVal > 5) ? calcVal : 5;

		if (currentFrame % explodeInterval == 0) {
			Vector3 pPos = GetWorldPosition();

			pPos.x += (std::rand() % 80 - 40) * 0.2f;
			pPos.y += (std::rand() % 80 - 40) * 0.2f;
			pPos.z += (std::rand() % 80 - 40) * 0.2f;

			if (gameScene_ && gameScene_->GetParticleManager()) {
				int pCount = 10 + static_cast<int>(animeTime_);
				gameScene_->GetParticleManager()->EmitStar(pPos, pCount, { 1.0f, 0.5f, 0.0f });
			}

			Audio::GetInstance()->SoundPlayWave(smallExplosionSound_);
		}

		if (animeTime_ >= 15.0f && !hasPlayedBigExplosion_) {
			if (gameScene_ && gameScene_->GetParticleManager()) {
				gameScene_->GetParticleManager()->EmitStar(GetWorldPosition(), 400, { 1.0f, 1.0f, 0.8f });
			}

			// 大爆発の瞬間にドカーンと鳴らす！
			Audio::GetInstance()->SoundPlayWave(bigExplosionSound_);

			// 実行したことを記録し、二度と入らないようにする
			hasPlayedBigExplosion_ = true;
		}

		break;
	}
	}

	// ==========================================
	// パーツ描画の基準座標を計算
	// ==========================================

	Vector3 basePos = worldTransform_.translation_;

	if (currentState_ == BossState::Intro) {
		float t = introTimer_ / 3.0f;
		float easeT = 1.0f - std::pow(1.0f - t, 3.0f);
		basePos.z += 150.0f * (1.0f - easeT);
		basePos.y += 30.0f * (1.0f - easeT);
	}
	else if (currentState_ == BossState::Dying) {
		float shakeIntensity = animeTime_ * 0.02f;

		if (shakeIntensity > 0.2f) {
			shakeIntensity = 0.2f;
		}

		basePos.x += (std::rand() % 10 - 5) * shakeIntensity;
		basePos.y += (std::rand() % 10 - 5) * shakeIntensity;
	}
	else {
		// 戦闘中：見た目を上下にフワフワさせる
		basePos.y += std::sin(animeTime_ * 1.5f) * 1.5f;
	}

	float baseRotY = 3.14159f;
	float flapAngle = std::sin(animeTime_ * 1.5f) * 0.15f;

	// 翼の羽ばたき
	for (int i = 0; i < 6; ++i) {
		transformWings_[i].translation_ = basePos;
		if (i < 3) {
			transformWings_[i].rotation_.y = flapAngle + baseRotY;
		}
		else {
			transformWings_[i].rotation_.y = -flapAngle + baseRotY;
		}
		transformWings_[i].matWorld_ = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, transformWings_[i].rotation_, transformWings_[i].translation_);
		transformWings_[i].TransferMatrix();
	}

	// 天使の輪
	transformHalo_.translation_ = basePos;
	transformHalo_.translation_.y += std::sin(animeTime_ * 2.0f) * 0.5f;
	transformHalo_.rotation_.y = baseRotY;
	transformHalo_.matWorld_ = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, transformHalo_.rotation_, transformHalo_.translation_);
	transformHalo_.TransferMatrix();

	// コアと金属パーツ
	transformCore_.translation_ = basePos;
	transformCore_.rotation_.y = baseRotY;
	transformCore_.matWorld_ = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, transformCore_.rotation_, transformCore_.translation_);
	transformCore_.TransferMatrix();

	for (int i = 0; i < 2; ++i) {
		if (!isArmorActive_[i]) continue;
		transformArmor_[i].translation_ = basePos;
		transformArmor_[i].rotation_.y = baseRotY;
		transformArmor_[i].matWorld_ = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, transformArmor_[i].rotation_, transformArmor_[i].translation_);
		transformArmor_[i].TransferMatrix();
	}
	for (int i = 0; i < 8; ++i) {
		if (!isTurretActive_[i]) continue;
		transformTurrets_[i].translation_ = basePos;
		transformTurrets_[i].rotation_.y = baseRotY;
		transformTurrets_[i].matWorld_ = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, transformTurrets_[i].rotation_, transformTurrets_[i].translation_);
		transformTurrets_[i].TransferMatrix();
	}

	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, basePos);
	worldTransform_.TransferMatrix();
}

void EnemyBoss::DrawNeon(const ViewProjection& viewProjection) {
	Vector3 dynamicLightDir = { std::sin(animeTime_), -1.0f, std::cos(animeTime_) };
	if (currentState_ == BossState::Dying && animeTime_ >= 15.0f) return;

	for (int i = 0; i < 2; ++i) {
		if (armorModels_[i] && isArmorActive_[i]) {
			armorModels_[i]->SetColor(armorColor_[0], armorColor_[1], armorColor_[2], 1.0f);
			armorModels_[i]->SetLight(1.0f, 1.0f, 1.0f, 1.0f, dynamicLightDir);
			armorModels_[i]->Draw(transformArmor_[i], viewProjection, whiteTexture_);
		}
	}
	for (int i = 0; i < 8; ++i) {
		if (turretModels_[i] && isTurretActive_[i]) {
			turretModels_[i]->SetColor(turretColor_[0], turretColor_[1], turretColor_[2], 1.0f);
			turretModels_[i]->SetLight(1.0f, 1.0f, 1.0f, 1.0f, dynamicLightDir);
			turretModels_[i]->Draw(transformTurrets_[i], viewProjection, whiteTexture_);
		}
	}

	for (int i = 0; i < 6; ++i) {
		if (wingModels_[i]) {
			wingModels_[i]->SetNeonColor(wingIntensity_, wingColor_[0], wingColor_[1], wingColor_[2]);
			wingModels_[i]->Draw(transformWings_[i], viewProjection, whiteTexture_);
		}
	}

	if (haloModel_) {
		haloModel_->SetNeonColor(haloIntensity_, haloColor_[0], haloColor_[1], haloColor_[2]);
		haloModel_->Draw(transformHalo_, viewProjection, whiteTexture_);
	}

	if (flashTimer_ > 0) {
		if (coreModel_) {
			coreModel_->SetNeonColor(30.0f, 1.0f, 1.0f, 1.0f);
			coreModel_->Draw(transformCore_, viewProjection, whiteTexture_);
		}
	}
	else {
		if (coreModel_) {
			coreModel_->SetNeonColor(coreIntensity_, coreColor_[0], coreColor_[1], coreColor_[2]);
			coreModel_->Draw(transformCore_, viewProjection, whiteTexture_);
		}
	}

	if (pixelModel_) {
		pixelModel_->SetNeonColor(20.0f, 1.0f, 0.2f, 0.2f); // 赤系の高輝度
		for (int i = 0; i < kMaxChargePixels; ++i) {
			if (chargePixels_[i].isActive) {
				pixelModel_->Draw(pixelTransforms_[i], viewProjection, whiteTexture_);
			}
		}
	}

	//極太レーザーの描画
	if (isLaserActive_ && laserModel_) {
		laserModel_->SetNeonColor(30.0f, 1.0f, 0.0f, 0.0f); // 真っ赤な極太レーザー
		laserModel_->Draw(transformLaser_, viewProjection, whiteTexture_);
	}

	if (haloModel_) {
		for (int i = 0; i < 8; ++i) {
			if (shockwaveRings_[i].isActive) {
				haloModel_->SetNeonColor(20.0f, 1.0f, 0.8f, 0.2f);
				haloModel_->Draw(shockwaveRings_[i].transform, viewProjection, whiteTexture_);
			}
		}
	}
}

void EnemyBoss::OnCollision() {
	if (currentState_ == BossState::Intro || currentState_ == BossState::Dying) return;
	if (hitTimer_ > 0) return;

	if (player_) {
		Vector3 pPos = player_->GetWorldPosition();
		Vector3 tPos = GetWorldPosition();
		float dx = pPos.x - tPos.x;
		float dy = pPos.y - tPos.y;
		float dz = pPos.z - tPos.z;
		float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
		if (dist <= GetRadius() + 2.0f) return;
	}

	currentHp_--;
	hitTimer_ = 4;
	flashTimer_ = 6;

	if (gameScene_ && gameScene_->GetParticleManager()) {
		gameScene_->GetParticleManager()->EmitStar(GetWorldPosition(), 5, { 1.0f, 0.8f, 0.2f });
	}

	if (currentHp_ <= 0) {
		currentHp_ = 0;
		currentState_ = BossState::Dying;
		animeTime_ = 0.0f; // 死亡演出用のタイマーとしてリセット
		hasPlayedBigExplosion_ = false;
	}
}



void EnemyBoss::AttackTurret() {
	attackTimer_ += 1.0f / 60.0f;

	if (turretFireCount_ < 8) {
		if (attackTimer_ >= 0.15f) {
			EnemyBullet* bullet = new EnemyBullet();
			bullet->Initialize(bulletModel_, { 0,0,0 }, { 0,0,0 }, dummyTexture_);
			bullet->SetStandby(true); // 待機モードで出す
			bullet->SetPlayer(player_);

			standbyBullets_[turretFireCount_] = bullet;
			if (gameScene_) { gameScene_->AddEnemyBullet(bullet); }

			turretFireCount_++;
			attackTimer_ = 0.0f;
		}
	}
	//  8個出揃ってから0.8秒後に、一斉に発射！
	else if (attackTimer_ >= 0.8f) {
		Vector3 playerPos = player_->GetWorldPosition();
		for (int i = 0; i < 8; ++i) {
			if (standbyBullets_[i]) {
				Vector3 bPos = standbyBullets_[i]->GetWorldPosition();
				Vector3 velocity = { playerPos.x - bPos.x, playerPos.y - bPos.y, playerPos.z - bPos.z };

				float speed = 1.0f;
				float length = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
				if (length > 0) {
					velocity.x = (velocity.x / length) * speed;
					velocity.y = (velocity.y / length) * speed;
					velocity.z = (velocity.z / length) * speed;
				}

				standbyBullets_[i]->SetVelocity(velocity);
				standbyBullets_[i]->SetStandby(false); // 待機解除で発射！
				standbyBullets_[i] = nullptr;
			}
		}
		attackPhase_ = 1; // クールダウンフェーズへ
		turretFireCount_ = 0;
		attackTimer_ = 0.0f;
	}

	//  待機中の弾を、フワフワ動くタレットの砲口にピッタリ合わせ続ける
	Vector3 basePos = GetWorldPosition();
	for (int i = 0; i < 8; ++i) {
		if (standbyBullets_[i]) {
			float angle = (3.14159f * 2.0f / 8.0f) * i;
			float radius = 15.0f; // タレットの半径
			Vector3 turretPos = {
				basePos.x + std::cos(angle) * radius,
				basePos.y + std::sin(angle) * radius,
				basePos.z
			};
			standbyBullets_[i]->SetPosition(turretPos);
		}
	}
}

void EnemyBoss::AttackMissile() {
	// 追尾ミサイルの処理をここに書く
}

void EnemyBoss::AttackLaser() {
	attackTimer_ += 1.0f / 60.0f;
	Vector3 basePos = GetWorldPosition();

	//  チャージフェーズ
	if (attackTimer_ < 2.0f) {

		if (!hasPlayedChargeSound_) {
			Audio::GetInstance()->SoundPlayWave(chargeSound_);
			hasPlayedChargeSound_ = true;
		}

		isLaserActive_ = false;

		// チャージ中はボスを激しく振動させる
		worldTransform_.translation_.x += (std::rand() % 10 - 5) * 0.05f;
		if (std::fmod(attackTimer_, 0.2f) < 0.1f) flashTimer_ = 2; // コア明滅

		// ピクセルを周囲にランダム発生させる
		for (int i = 0; i < 2; ++i) { // 毎フレーム2個ずつ出す
			for (int j = 0; j < kMaxChargePixels; ++j) {
				if (!chargePixels_[j].isActive) {
					chargePixels_[j].isActive = true;
					float angleX = (rand() % 360) * 3.14159f / 180.0f;
					float angleY = (rand() % 360) * 3.14159f / 180.0f;
					float distance = 40.0f + (rand() % 20); // 遠くから発生

					chargePixels_[j].position = {
						basePos.x + std::cos(angleY) * std::sin(angleX) * distance,
						basePos.y + std::sin(angleY) * distance,
						basePos.z + std::cos(angleY) * std::cos(angleX) * distance
					};
					break;
				}
			}
		}

		// プレイヤーの方向を計算してロックオン
		if (player_) {
			Vector3 pPos = player_->GetWorldPosition();
			transformLaser_.rotation_.y = std::atan2(pPos.x - basePos.x, pPos.z - basePos.z) + 1.5708f;
		}
	}
	// レーザー発射フェーズ
	else if (attackTimer_ < 3.0f) {

		if (!hasPlayedBeamSound_) {
			Audio::GetInstance()->SoundPlayWave(beamSound_);
			hasPlayedBeamSound_ = true;
		}

		isLaserActive_ = true;
		flashTimer_ = 2;

		// チャージエフェクトはすべて消す
		for (int i = 0; i < kMaxChargePixels; ++i) chargePixels_[i].isActive = false;


		if (player_) {
			Vector3 pPos = player_->GetWorldPosition();
			float targetAngle = std::atan2(pPos.x - basePos.x, pPos.z - basePos.z) + 1.5708f;

			// プレイヤーの方へ少しずつ角度を向ける
			float diff = targetAngle - transformLaser_.rotation_.y;
			while (diff > 3.14159f) diff -= 3.14159f * 2.0f;
			while (diff < -3.14159f) diff += 3.14159f * 2.0f;
			transformLaser_.rotation_.y += diff * 0.1f; 
		}

		// レーザーのスケール演出
		float laserTime = attackTimer_ - 2.0f;
		float lengthX = laserTime * 300.0f;
		if (lengthX > 150.0f) lengthX = 150.0f;
		float thickness = 2.5f - (laserTime * 1.0f);

		transformLaser_.scale_ = { lengthX, thickness, thickness };


		float currentAngle = transformLaser_.rotation_.y - 1.5708f;
		Vector3 dir = { std::sin(currentAngle), 0.0f, std::cos(currentAngle) };

		float offset = (lengthX / 2.0f) + 3.0f;
		transformLaser_.translation_ = {
			basePos.x + dir.x * offset,
			basePos.y,
			basePos.z + dir.z * offset
		};

		transformLaser_.matWorld_ = MakeAffineMatrix(transformLaser_.scale_, transformLaser_.rotation_, transformLaser_.translation_);
		transformLaser_.TransferMatrix();

		// プレイヤーへのダメージ判定
		if (player_ && CheckLaserCollision(player_->GetWorldPosition())) {
			player_->OnCollision();
		}
	}
	// 終了フェーズ
	else {

		hasPlayedChargeSound_ = false;
		hasPlayedBeamSound_ = false;
		isLaserActive_ = false;
		attackPhase_ = 3; // クールダウン待機フェーズへ移行
		attackTimer_ = 0.0f;
	}

	// ==========================================
	// チャージ中ピクセルの吸い込み更新処理
	// ==========================================
	for (int i = 0; i < kMaxChargePixels; ++i) {
		if (chargePixels_[i].isActive) {
			chargePixels_[i].position.x += (basePos.x - chargePixels_[i].position.x) * 0.15f;
			chargePixels_[i].position.y += (basePos.y - chargePixels_[i].position.y) * 0.15f;
			chargePixels_[i].position.z += (basePos.z - chargePixels_[i].position.z) * 0.15f;

			float dx = basePos.x - chargePixels_[i].position.x;
			float dy = basePos.y - chargePixels_[i].position.y;
			float dz = basePos.z - chargePixels_[i].position.z;
			if (std::sqrt(dx * dx + dy * dy + dz * dz) < 2.0f) {
				chargePixels_[i].isActive = false;
			}

			pixelTransforms_[i].translation_ = chargePixels_[i].position;
			pixelTransforms_[i].rotation_.x += 0.3f;
			pixelTransforms_[i].rotation_.y += 0.3f;
			pixelTransforms_[i].scale_ = { 0.4f, 0.4f, 0.4f };
			pixelTransforms_[i].matWorld_ = MakeAffineMatrix(pixelTransforms_[i].scale_, pixelTransforms_[i].rotation_, pixelTransforms_[i].translation_);
			pixelTransforms_[i].TransferMatrix();
		}
	}
}

// ==========================================
//レーザー当たり判定
// ==========================================
bool EnemyBoss::CheckLaserCollision(const Vector3& targetPos) {
	Vector3 center = transformLaser_.translation_;
	Vector3 xAxis = {
		transformLaser_.matWorld_.m[0][0],
		transformLaser_.matWorld_.m[0][1],
		transformLaser_.matWorld_.m[0][2]
	};

	float lengthRatio = 1.0f;
	Vector3 start = { center.x - xAxis.x * lengthRatio, center.y - xAxis.y * lengthRatio, center.z - xAxis.z * lengthRatio };
	Vector3 end = { center.x + xAxis.x * lengthRatio, center.y + xAxis.y * lengthRatio, center.z + xAxis.z * lengthRatio };

	Vector3 v = { end.x - start.x, end.y - start.y, end.z - start.z };
	Vector3 w = { targetPos.x - start.x, targetPos.y - start.y, targetPos.z - start.z };

	float c1 = w.x * v.x + w.y * v.y + w.z * v.z;
	float c2 = v.x * v.x + v.y * v.y + v.z * v.z;

	if (c2 == 0.0f) return false;

	Vector3 closest;
	if (c1 <= 0.0f) { closest = start; }
	else if (c2 <= c1) { closest = end; }
	else {
		float b = c1 / c2;
		closest = { start.x + v.x * b, start.y + v.y * b, start.z + v.z * b };
	}

	Vector3 diff = { targetPos.x - closest.x, targetPos.y - closest.y, targetPos.z - closest.z };
	diff.z *= 0.2f; // Z方向の距離を1/5に圧縮（元の仕様をそのまま採用）

	float distance = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

	// プレイヤーの半径(例:2.0f) ＋ レーザーの現在の太さ(scale_.y) で当たり判定
	return distance < (2.0f + transformLaser_.scale_.y);
}

void EnemyBoss::AttackRing() {
	attackTimer_ += 1.0f / 60.0f;


	if (turretFireCount_ < 8) {
		if (attackTimer_ >= 0.2f) {
			shockwaveRings_[turretFireCount_].isActive = true;

			Vector3 pPos = player_ ? player_->GetWorldPosition() : GetWorldPosition();

			// 🌟 修正：画面外に落ちないよう、散らばる範囲を少しだけ絞りました（-40 ～ 40）
			float offsetX = (std::rand() % 80 - 40) * 1.0f;
			float offsetZ = (std::rand() % 80 - 40) * 1.0f;

			shockwaveRings_[turretFireCount_].transform.translation_ = {
				pPos.x + offsetX,
				35.0f,
				pPos.z + offsetZ
			};

			shockwaveRings_[turretFireCount_].transform.rotation_ = { 0.0f, 0.0f, 0.0f };
			shockwaveRings_[turretFireCount_].transform.scale_ = { 10.0f, 1.0f, 10.0f };

			turretFireCount_++;  // 次のリングへ！
			attackTimer_ = 0.0f; // 次を出すためにタイマーリセット
		}

		// ボスの予備動作（フワッと上に浮く）
		worldTransform_.translation_.y += (20.0f - worldTransform_.translation_.y) * 0.05f;
	}
	// 🎬 ② 8個すべて出し終わった後の待機＆戻り動作
	else {
		// 全て出し終わってから、さらにタイマーが経過する
		if (attackTimer_ < 1.0f) {
			worldTransform_.translation_.y += (0.0f - worldTransform_.translation_.y) * 0.1f; // 定位置に戻る
		}

		// 🎬 ③ 落下し切るのを待ってから次のフェーズへ
		if (attackTimer_ >= 3.0f) {
			attackPhase_ = 5;
			attackTimer_ = 0.0f;
			turretFireCount_ = 0; // カウントを元に戻す
		}
	}
}


bool EnemyBoss::CheckRingCollision(const Vector3& targetPos, const ShockwaveRing& ring) {
	float diffY = std::abs(targetPos.y - ring.transform.translation_.y);
	if (diffY < 2.0f) {

		float dx = targetPos.x - ring.transform.translation_.x;
		float dz = targetPos.z - ring.transform.translation_.z;
		float dist = std::sqrt(dx * dx + dz * dz);


		float currentRadius = ring.transform.scale_.x * 3.8f;

		float ringThickness = 2.0f;

		// プレイヤーが「輪っかの線の上」にいる時だけダメージ
		if (dist > currentRadius - ringThickness && dist < currentRadius + ringThickness) {
			return true;
		}
	}
	return false;
}

void EnemyBoss::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::PushID(this);
	ImGui::Text("=== SERAPH BOSS STATUS ===");
	ImGui::Text("HP: %d / %d", currentHp_, maxHp_);
	ImGui::SliderInt("Max HP", &maxHp_, 100, 2000);

	ImGui::Separator();
	ImGui::Text("--- Metal Parts Colors ---");
	ImGui::ColorEdit3("Armor Color", armorColor_);
	ImGui::ColorEdit3("Turret Color", turretColor_);

	ImGui::Separator();
	ImGui::Text("--- Neon Parts Colors ---");
	ImGui::ColorEdit3("Core Color", coreColor_);
	ImGui::SliderFloat("Core Intensity", &coreIntensity_, 0.0f, 30.0f);

	ImGui::ColorEdit3("Wing Color", wingColor_);
	ImGui::SliderFloat("Wing Intensity", &wingIntensity_, 0.0f, 30.0f);

	ImGui::ColorEdit3("Halo Color", haloColor_);
	ImGui::SliderFloat("Halo Intensity", &haloIntensity_, 0.0f, 30.0f);

	ImGui::SliderInt("Max HP", &maxHp_, 1, 2000);

	ImGui::SameLine(); // スライダーの横にボタンを並べる
	if (ImGui::Button("Debug: HP to 1")) {
		currentHp_ = 1;
	}

	ImGui::Separator();

	if (ImGui::Button("SAVE BOSS SETTINGS (JSON)")) {
		GlobalVariables* global = GlobalVariables::GetInstance();
		const std::string groupName = "BossSeraphSettings";

		global->SetValue(groupName, "MaxHP", maxHp_);
		global->SetValue(groupName, "ArmorColor", Vector3(armorColor_[0], armorColor_[1], armorColor_[2]));
		global->SetValue(groupName, "TurretColor", Vector3(turretColor_[0], turretColor_[1], turretColor_[2]));
		global->SetValue(groupName, "CoreColor", Vector3(coreColor_[0], coreColor_[1], coreColor_[2]));
		global->SetValue(groupName, "CoreIntensity", coreIntensity_);
		global->SetValue(groupName, "WingColor", Vector3(wingColor_[0], wingColor_[1], wingColor_[2]));
		global->SetValue(groupName, "WingIntensity", wingIntensity_);
		global->SetValue(groupName, "HaloColor", Vector3(haloColor_[0], haloColor_[1], haloColor_[2]));
		global->SetValue(groupName, "HaloIntensity", haloIntensity_);

		global->SaveFile(groupName);
	}

	ImGui::PopID();
#endif
}