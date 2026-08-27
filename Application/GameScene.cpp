#include "GameScene.h"
#include "Input/Input.h"
#include "2D/TextureManager.h"
#include "Application/Character/Player.h"
#include "AxisIndicator.h"
#include "GlobalValiables.h"
#include "WindowApp.h"
#include <cmath>
#include "CollisionManager.h"
#include "Skydome.h"
#include "WaveManager.h"
#include "WeakEnemyCross.h"
#include "WeakEnemySpinCore.h"
#include "WeakEnemyTriangle.h"
#include "WarningUI.h"
#include "EnemyBoss.h"
#include "ResultUI.h"

#include "BaseEnemy.h"
#include <random>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

GameScene::~GameScene() {
	delete debugCamera_;
	delete player_;
	for (BaseEnemy* enemy : enemies_) { delete enemy; }
	for (EnemyBullet* bullet : enemyBullets_) { delete bullet; }
	delete collisionManager_;
	delete skydomeModel_;
	delete skydome_;
	delete groundModel_;
	delete railEditor_;
	delete waveManager_;
	delete resultUI_;
	delete titleUI_;
}

void GameScene::AddEnemyBullet(EnemyBullet* enemyBullet) {
	enemyBullets_.push_back(enemyBullet);
}




void GameScene::Initialize(DirectXCommon* dxCommon) {

	GlobalVariables::GetInstance()->LoadFiles();

	dxCommon_ = dxCommon;

	enemyObject_ = new Object3d();
	Object3d::StaticInitialize(dxCommon);
	NeonModel::StaticInitialize(dxCommon);
	BodyModel::StaticInitialize(dxCommon);

	/*-------------------------------
	ワールドトランスフォーム
	----------------------------------*/
	WorldTransform::SetDevice(dxCommon->GetDevice());

	// カメラの生成と初期化
	debugCamera_ = new DebugCamera();
	debugCamera_->Initialize();

	/*----------------------------
	ビュープロジェクションの初期化
	---------------------------------*/
	viewProjection_.Initialize();
	viewProjection_.translation_.z = -20.0f;
	viewProjection_.UpdateMatrix();

	/*-------------------------------
	3Dオブジェクトの生成と初期化
	----------------------------------*/
	/*----------------------
	地面
	-----------------------------*/
	groundModel_ = new Object3d();
	groundModel_->Initialize("Resources/Ground", "ground.obj");
	groundTex_ = TextureManager::Load("Resources/Ground/ground.png");

	groundTransform_.Initialize();
	groundTransform_.scale_ = { 1.0f, 1.0f, 1.0f };
	groundTransform_.rotation_ = { 0.0f, 0.0f, 0.0f };
	groundTransform_.translation_ = { 0.0f, -15.0f, 0.0f }; // 原点に配置
	groundTransform_.matWorld_ = MakeAffineMatrix(groundTransform_.scale_, groundTransform_.rotation_, groundTransform_.translation_);
	groundTransform_.TransferMatrix();





	myNeonBar_ = new NeonObj();
	myNeonBar_->Initialize("Resources/Neon", "Neon_bar.obj");

	neonModel_ = new NeonModel();


	bloom_ = new Bloom();
	bloom_->Initialize(dxCommon_, 1280, 720);

	// エネミー
	BaseEnemy::StaticInitialize();
	WeakEnemyCross::StaticInitialize();
	WeakEnemySpinCore::StaticInitialize();
	WeakEnemyTriangle::StaticInitialize();

	/*-----------------------
	天球の生成と初期化
	-----------------------------*/
	skydomeModel_ = new Object3d();
	skydomeModel_->Initialize("Resources/skyDome", "AL3_skyDome.obj");
	skydomeTex_ = TextureManager::Load("Resources/skyDome/AL3_skydome.png");

	skydome_ = new Skydome();
	skydome_->Initialize(skydomeModel_, skydomeTex_);




	/*-------------------------------
	自キャラ生成と初期化
	----------------------------------*/
	Player::RegisterGlobalVariables();
	player_ = new Player();
	player_->Initialize();
	player_->SetEnemies(&enemies_);
	TextureManager::Load("Resources/ring.png");

	/*------------------------------
	waveManager
	------------------------------*/
	waveManager_ = new WaveManager();
	waveManager_->Initialize();

	/*-------------------------
	衝突マネージャー
	------------------------------*/
	collisionManager_ = new CollisionManager();

	/*------------------------------
	レールカメラ
	-------------------------------------*/
	rail_ = new Rail();
	rail_->Initialize();

	railCamera_ = new RailCamera();
	railCamera_->Initialize(rail_);

	viewProjection_ = railCamera_->GetViewProjection();

	// editor 
	railEditor_ = new RailEditor();
	railEditor_->Initialize(rail_);

	/*-----------------------
	カメラシェイク
	-----------------------------*/
	cameraShake_ = new CameraShake();
	cameraShake_->Initialize();
	player_->SetCameraShake(cameraShake_);

	/*-----------------------
	軸表示
	------------------------*/
	AxisIndicator::GetInstance()->Initialize();
	AxisIndicator::GetInstance()->SetVisible(true);
	AxisIndicator::GetInstance()->SetTargetCamera(&viewProjection_);

	/*-----------------
	ワープエフェクト
	-----------------------*/
	warpEffect_ = new WarpEffect();
	warpEffect_->Initialize(dxCommon_);

	/*----------------------------
	UI表示
	----------------------*/

	// ボス警告
	WarningUI::GetInstance()->Initialize("Resources/UI");

	// リザルト
	resultUI_ = new ResultUI();
	resultUI_->Initialize("Resources/UI", dxCommon);

	// タイトル
	titleUI_ = new TitleUI();
	titleUI_->Initialize(dxCommon);

	// ボスHP
	bossUI_ = new BossUI();
	bossUI_->Initialize();

	// スコア
	uint32_t whiteTex = TextureManager::Load("Resources/white.png");
	uint32_t blackTex = TextureManager::Load("Resources/black.png");
	scoreUI_ = new ScoreUI();
	scoreUI_->Initialize(dxCommon_, whiteTex);

	lifeUI_ = new LifeUI();
	lifeUI_->Initialize(dxCommon_, whiteTex);

	waveUI_ = new WaveUI();
	waveUI_->Initialize(dxCommon_, whiteTex);

	// 死んだとき
	rebootUI_ = new RebootUI();
	rebootUI_->Initialize(dxCommon_, whiteTex);

	// チュートリアル
	tutorialUI_ = new TutorialUI();
	tutorialUI_->Initialize(dxCommon_, whiteTex,blackTex);

	/*------------------------
	パーティクル
	--------------------------*/
	particleModel_ = new NeonModel();
	particleModel_->Initialize("Resources", "block.obj");


	NeonModel* starModel = new NeonModel();
	starModel->Initialize("Resources", "Star.obj");

	particleManager_ = new ParticleManager();
	particleManager_->Initialize(particleModel_, starModel, TextureManager::Load("Resources/white.png"));
	/*-----------------------
	シーン
	------------------------------*/
	sceneState_ = SceneState::Title;
	sceneTimer_ = 0.0f;

	warpLaserModel_ = new NeonModel();
	warpLaserModel_->Initialize("Resources/Enemy/Neon", "Laser.obj");

	warpStarModel_ = new NeonModel();
	warpStarModel_->Initialize("Resources", "Star.obj");

	warpLaserL_.Initialize();
	warpLaserR_.Initialize();
	warpStarTf_.Initialize();


	for (int i = 0; i < kMaxAmbientParticles; ++i) {
		ambientTransforms_[i].Initialize();

		float spawnX = (std::rand() % 200 - 100) * 1.0f;
		float spawnY = (std::rand() % 120 - 60) * 1.0f;

		if (spawnX > -40.0f && spawnX < 40.0f) spawnX = (spawnX > 0) ? spawnX + 40.0f : spawnX - 40.0f;
		if (spawnY > -30.0f && spawnY < 30.0f) spawnY = (spawnY > 0) ? spawnY + 30.0f : spawnY - 30.0f;

		ambientParticles_[i].position = { spawnX, spawnY, (std::rand() % 250) * 1.0f };

		ambientParticles_[i].rotation = {
			(std::rand() % 360) * 3.14159f / 180.0f,
			(std::rand() % 360) * 3.14159f / 180.0f,
			(std::rand() % 360) * 3.14159f / 180.0f
		};

		ambientParticles_[i].scale = (std::rand() % 5 + 2) * 0.05f;

		ambientParticles_[i].rotSpeed = {
			(std::rand() % 100 - 50) * 0.0005f,
			(std::rand() % 100 - 50) * 0.0005f,
			(std::rand() % 100 - 50) * 0.0005f
		};
		ambientParticles_[i].zSpeed = (std::rand() % 10 + 2) * 0.05f;

		int colorType = std::rand() % 3;
		if (colorType == 0) ambientParticles_[i].color = { 1.0f, 0.2f, 0.8f }; // ピンク
		else if (colorType == 1) ambientParticles_[i].color = { 0.0f, 0.8f, 1.0f }; // シアン
		else ambientParticles_[i].color = { 0.6f, 0.2f, 1.0f }; // 紫

		ambientParticles_[i].intensity = (std::rand() % 30 + 20) * 0.1f;
	}


	/*-------------------------
	音
	-------------------------------*/
	bgmSound_ = Audio::GetInstance()->SoundLoadWave("Sounds/mainBGM.wav");
	warpSound_ = Audio::GetInstance()->SoundLoadWave("Sounds/warp.wav");

	bgmVoice_ = Audio::GetInstance()->SoundPlayWave(bgmSound_, true);

}

void GameScene::Update() {



	// ==========================================
//  ImGuiの描画
// ==========================================
#ifdef USE_IMGUI

	ImGui::Begin("Master Control", nullptr, ImGuiWindowFlags_MenuBar);

	//  自機の設定
	if (ImGui::TreeNodeEx("Player Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		if (player_) player_->DrawImGui();
		ImGui::TreePop();
	}

	// 敵
	if (ImGui::TreeNodeEx("Enemy Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		int i = 0;
		for (BaseEnemy* enemy : enemies_) { // ✨ 変更
			ImGui::PushID(i);
			if (ImGui::TreeNode((std::string("Enemy ") + std::to_string(i)).c_str())) {
				enemy->DrawImGui();
				ImGui::TreePop();
			}
			ImGui::PopID();
			i++;
		}
		ImGui::TreePop();
	}

	// ネオン文字の設定
	if (ImGui::TreeNodeEx("Neon Text Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::ColorEdit3("Text Color", neonColor_);
		ImGui::SliderFloat("Text Intensity", &neonIntensity_, 0.0f, 20.0f);
		ImGui::SliderFloat("Text Radius", &neonRadius_, 0.001f, 0.1f);
		ImGui::SliderFloat("Text Softness", &neonSoftness_, 0.1f, 50.0f);
		ImGui::SliderFloat("Text Length Offset", &neonLengthOffset_, -1.0f, 1.0f);

		ImGui::Separator();
		ImGui::Checkbox("Enable Flicker", &neonTextFlicker_.isFlicker_);
		ImGui::TreePop();
	}

	// ネオンバーの設定
	if (myNeonBar_ != nullptr) {
		myNeonBar_->DrawImGui("Neon Bar Settings");
		if (myNeonBar_->GetModel() != nullptr) {
			myNeonBar_->GetModel()->DrawImGui("Neon Bar - Plasma Settings");
		}
	}

	// Bloomの設定
	if (bloom_ != nullptr) {
		bloom_->DrawImGui();
	}

	// グローバル変数の設定
	if (ImGui::TreeNodeEx("Global Variables")) {
		GlobalVariables::GetInstance()->Update();
		ImGui::TreePop();
	}

	WarningUI::GetInstance()->DrawImGui();

	// WaveManager
	waveManager_->DrawImGui();

	if (resultUI_) {
		resultUI_->DrawImGui();
	}

	if (titleUI_) {
		titleUI_->DrawImGui();
	}

	if (scoreUI_) {
		scoreUI_->DrawImGui();
	}

	if (lifeUI_) {
		lifeUI_->DrawImGui();
	}

	if (waveUI_) {
		waveUI_->DrawImGui();
	}

	if (rebootUI_) {
		rebootUI_->DrawImGui();
	}

	if (tutorialUI_) {
		tutorialUI_->DrawImGui();
	}

	ImGui::End(); // Master Controlの終了

#endif

	if (titleUI_) {
		titleUI_->SetActive(sceneState_ == SceneState::Title || sceneState_ == SceneState::StartWarp);
		titleUI_->Update(1.0f / 60.0f);
	}

	if (resultUI_ && resultUI_->IsActive()) {
		resultUI_->Update(1.0f / 60.0f);
	}

	if (sceneState_ == SceneState::Playing) {
		playTime_ += 1.0f / 60.0f; // 毎フレーム時間を足す
	}

	if (lifeUI_ && sceneState_ == SceneState::Playing) {
		lifeUI_->Update();
	}

	if (waveUI_ && sceneState_ == SceneState::Playing) {
		int currentWave = waveManager_->GetCurrentWave() - 1;
		bool isInterval = !waveManager_->IsWaveActive();
		if (isInterval) { currentWave += 1; }
		if (currentWave < 1) { currentWave = 1; }
		waveUI_->Update(currentWave, isInterval);

		if (tutorialUI_) {
			tutorialUI_->Update(currentWave, isInterval);
		}



	}

	if (rebootUI_ && rebootUI_->IsActive() && sceneState_ == SceneState::Playing) {
		sceneState_ = SceneState::Rebooting;

		for (BaseEnemy* enemy : enemies_) {
			if (particleManager_) {
				particleManager_->Emit(enemy->GetTranslation(), 80, { 1.0f, 0.0f, 0.2f });
			}
			delete enemy;
		}
		enemies_.clear();

		for (EnemyBullet* bullet : enemyBullets_) { delete bullet; }
		enemyBullets_.clear();
	}
	/*------------------------
	死亡検知とディレイ処理
	--------------------*/
	if (player_) {
		bool isPlayerDead = player_->IsDead();
		if (isPlayerDead && !wasPlayerDead_) {
			if (lifeUI_) lifeUI_->DecreaseLife();

			if (lifeUI_->GetLife() <= 0) {
			
				deathTimer_ = 90.0f;
			}
		}
		wasPlayerDead_ = isPlayerDead;
	}

	if (lifeUI_ && lifeUI_->GetLife() <= 0 && sceneState_ == SceneState::Playing) {
		deathTimer_ -= 1.0f;
		if (deathTimer_ <= 0.0f) {
			sceneState_ = SceneState::Rebooting;
			if (rebootUI_) rebootUI_->Start();

			Audio::GetInstance()->SoundStopWave(bgmVoice_);
			bgmVoice_ = nullptr;

			for (BaseEnemy* enemy : enemies_) {
				if (particleManager_) {
					// 敵の位置から、赤色のパーティクルを大量発生
					particleManager_->Emit(enemy->GetTranslation(), 80, { 1.0f, 0.0f, 0.2f });
				}
				delete enemy;
			}
			enemies_.clear();

			for (EnemyBullet* bullet : enemyBullets_) { delete bullet; }
			enemyBullets_.clear();
		}
	}

	if (sceneState_ != SceneState::Playing) {
		sceneTimer_ += 1.0f / 60.0f;
		float t = sceneTimer_;

		if (particleManager_) particleManager_->Update();

		float currentWarpIntensity = 10.0f;
		Matrix4x4 warpParent = MakeIdentity4x4();


		/*---------------------------
				タイトル
				--------------------------------*/
		if (sceneState_ == SceneState::Title) {
			currentWarpIntensity = 15.0f;


			if (player_) {
				player_->GetWorldTransform().translation_ = { 0.0f, -2.0f + std::sin(t * 2.0f) * 0.5f, 15.0f };
				player_->GetWorldTransform().rotation_ = { 0.0f, 0.0f, std::sin(t * 1.5f) * 0.1f };
				player_->Update(MakeIdentity4x4());
			}

			// カメラは自機(0, -2, 15)を中心に回るように計算
			float radius = 25.0f;
			float angle = std::sin(t * 0.5f) * 0.5f;
			float camX = 0.0f + std::sin(angle) * radius;
			float camZ = 15.0f - std::cos(angle) * radius;
			float camY = -2.0f + 4.0f + std::sin(t * 0.3f) * 3.0f;

			viewProjection_.translation_ = { camX, camY, camZ };

			Vector3 forward = { 0.0f - camX, -2.0f - camY, 15.0f - camZ };
			float xzLen = std::sqrt(forward.x * forward.x + forward.z * forward.z);
			viewProjection_.rotation_.y = std::atan2(forward.x, forward.z);
			viewProjection_.rotation_.x = std::atan2(-forward.y, xzLen);
			viewProjection_.rotation_.z = 0.0f;
			viewProjection_.UpdateMatrix();

			if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
				sceneState_ = SceneState::StartWarp;
				sceneTimer_ = 0.0f;
				warpCamStartPos_ = viewProjection_.translation_;
				if (titleUI_) titleUI_->PlayStartAnimation();

				warpVoice_ = Audio::GetInstance()->SoundPlayWave(warpSound_);
			}
		}
		// ゲーム開始の超加速＆シームレスカメラ
		else if (sceneState_ == SceneState::StartWarp) {
			float ease = t / 2.0f;
			if (ease > 1.0f) ease = 1.0f;
			float zoomEase = std::pow(ease, 4.0f);

			float camMoveEase = t / 0.8f;
			if (camMoveEase > 1.0f) camMoveEase = 1.0f;
			float smoothCam = camMoveEase * camMoveEase * (3.0f - 2.0f * camMoveEase);


			float targetZ = 15.0f - 20.0f - (zoomEase * 10.0f);
			viewProjection_.translation_.x = warpCamStartPos_.x + (0.0f - warpCamStartPos_.x) * smoothCam;
			viewProjection_.translation_.y = warpCamStartPos_.y + (0.0f - warpCamStartPos_.y) * smoothCam;
			viewProjection_.translation_.z = warpCamStartPos_.z + (targetZ - warpCamStartPos_.z) * smoothCam;

			float pZ = 15.0f + (zoomEase * 400.0f); // 自機の前進
			Vector3 playerPos = { 0.0f, -2.0f, pZ };
			Vector3 forward = { playerPos.x - viewProjection_.translation_.x,
								playerPos.y - viewProjection_.translation_.y,
								playerPos.z - viewProjection_.translation_.z };
			float xzLen = std::sqrt(forward.x * forward.x + forward.z * forward.z);
			viewProjection_.rotation_.y = std::atan2(forward.x, forward.z);
			viewProjection_.rotation_.x = std::atan2(-forward.y, xzLen);
			viewProjection_.rotation_.z = 0.0f;

			if (player_) {
				player_->GetWorldTransform().translation_ = playerPos;
				player_->GetWorldTransform().rotation_ = { 0.0f, 0.0f, 0.0f };
				player_->Update(MakeIdentity4x4());

				float beamLen = 50.0f + (zoomEase * 800.0f);
				warpLaserL_.scale_ = { beamLen, 0.3f, 0.3f };
				warpLaserL_.rotation_ = { 0.0f, 1.5708f, 0.0f };
				warpLaserL_.translation_ = { -4.5f, -2.0f, pZ - (beamLen / 2.0f) };
				warpLaserL_.matWorld_ = MakeAffineMatrix(warpLaserL_.scale_, warpLaserL_.rotation_, warpLaserL_.translation_);
				warpLaserL_.TransferMatrix();

				warpLaserR_.scale_ = { beamLen, 0.3f, 0.3f };
				warpLaserR_.rotation_ = { 0.0f, 1.5708f, 0.0f };
				warpLaserR_.translation_ = { 4.5f, -2.0f, pZ - (beamLen / 2.0f) };
				warpLaserR_.matWorld_ = MakeAffineMatrix(warpLaserR_.scale_, warpLaserR_.rotation_, warpLaserR_.translation_);
				warpLaserR_.TransferMatrix();
			}

			float shake = ease * 0.1f;
			viewProjection_.translation_.x += (std::rand() % 10 - 5) * shake;
			viewProjection_.translation_.y += (std::rand() % 10 - 5) * shake;
			viewProjection_.UpdateMatrix();

			currentWarpIntensity = 15.0f + (zoomEase * 300.0f);


			if (t >= 2.0f) {
				if (player_) {
					player_->GetWorldTransform().translation_ = { 0.0f, -2.0f, 15.0f }; // インゲームの定位置に戻す
					player_->Update(MakeIdentity4x4());
				}

				warpLaserL_.translation_.z = 9999.0f; warpLaserL_.TransferMatrix();
				warpLaserR_.translation_.z = 9999.0f; warpLaserR_.TransferMatrix();

				sceneState_ = SceneState::Playing;
				sceneTimer_ = 0.0f;

				if (warpVoice_) {
					Audio::GetInstance()->SoundStopWave(warpVoice_);
					warpVoice_ = nullptr;
				}

				if (bgmVoice_ == nullptr) {
					bgmVoice_ = Audio::GetInstance()->SoundPlayWave(bgmSound_, true);
				}
			}
		}
		else if (sceneState_ == SceneState::StartWarp) {
			float ease = t / 2.0f;
			if (ease > 1.0f) ease = 1.0f;
			float zoomEase = std::pow(ease, 4.0f);

			// カメラの移動
			float camMoveEase = t / 0.8f;
			if (camMoveEase > 1.0f) camMoveEase = 1.0f;
			float smoothCam = camMoveEase * camMoveEase * (3.0f - 2.0f * camMoveEase);

			float targetZ = -20.0f - (zoomEase * 10.0f);
			viewProjection_.translation_.x = warpCamStartPos_.x + (0.0f - warpCamStartPos_.x) * smoothCam;
			viewProjection_.translation_.y = warpCamStartPos_.y + (2.0f - warpCamStartPos_.y) * smoothCam;
			viewProjection_.translation_.z = warpCamStartPos_.z + (targetZ - warpCamStartPos_.z) * smoothCam;

			float pZ = zoomEase * 400.0f; // 自機の前進
			Vector3 playerPos = { 0.0f, 0.0f, pZ };
			Vector3 forward = { playerPos.x - viewProjection_.translation_.x,
								playerPos.y - viewProjection_.translation_.y,
								playerPos.z - viewProjection_.translation_.z };
			float xzLen = std::sqrt(forward.x * forward.x + forward.z * forward.z);
			viewProjection_.rotation_.y = std::atan2(forward.x, forward.z);
			viewProjection_.rotation_.x = std::atan2(-forward.y, xzLen);
			viewProjection_.rotation_.z = 0.0f;

			if (player_) {
				player_->GetWorldTransform().translation_ = playerPos;
				player_->GetWorldTransform().rotation_ = { 0.0f, 0.0f, 0.0f };
				player_->Update(MakeIdentity4x4());

				// 両脇のレーザー
				float beamLen = 50.0f + (zoomEase * 800.0f);
				warpLaserL_.scale_ = { beamLen, 0.3f, 0.3f };
				warpLaserL_.rotation_ = { 0.0f, 1.5708f, 0.0f };
				warpLaserL_.translation_ = { -4.5f, 0.0f, pZ - (beamLen / 2.0f) };
				warpLaserL_.matWorld_ = MakeAffineMatrix(warpLaserL_.scale_, warpLaserL_.rotation_, warpLaserL_.translation_);
				warpLaserL_.TransferMatrix();

				warpLaserR_.scale_ = { beamLen, 0.3f, 0.3f };
				warpLaserR_.rotation_ = { 0.0f, 1.5708f, 0.0f };
				warpLaserR_.translation_ = { 4.5f, 0.0f, pZ - (beamLen / 2.0f) };
				warpLaserR_.matWorld_ = MakeAffineMatrix(warpLaserR_.scale_, warpLaserR_.rotation_, warpLaserR_.translation_);
				warpLaserR_.TransferMatrix();
			}

			float shake = ease * 0.1f;
			viewProjection_.translation_.x += (std::rand() % 10 - 5) * shake;
			viewProjection_.translation_.y += (std::rand() % 10 - 5) * shake;
			viewProjection_.UpdateMatrix();

			currentWarpIntensity = 15.0f + (zoomEase * 300.0f);

			if (t >= 2.0f) {
				if (player_) {
					player_->GetWorldTransform().translation_ = { 0.0f, 0.0f, 0.0f };
					player_->Update(MakeIdentity4x4());
				}
				viewProjection_.translation_ = { 0.0f, 2.0f, -20.0f };
				viewProjection_.rotation_ = { 0.0f, 0.0f, 0.0f };
				viewProjection_.UpdateMatrix();

				warpLaserL_.translation_.z = 9999.0f; warpLaserL_.TransferMatrix();
				warpLaserR_.translation_.z = 9999.0f; warpLaserR_.TransferMatrix();

				sceneState_ = SceneState::Playing;
				sceneTimer_ = 0.0f;
			}
		}
		// クリア時の離脱ワープ演出
		else if (sceneState_ == SceneState::ClearWarp) {

			warpParent = MakeAffineMatrix({ 1.0f, 1.0f, 1.0f }, { 0, 0, 0 }, viewProjection_.translation_);

			// ボスの斜めカメラから真正面へ滑らかに移行
			if (t < 1.0f) {
				float ease = t / 1.0f;
				float smooth = ease * ease * (3.0f - 2.0f * ease);

				if (player_) {
					// 自機も滑らかに中央（X=0, Y=0, Z=0）へ戻す
					Vector3 pPos = player_->GetWorldPosition();
					pPos.x *= (1.0f - smooth);
					pPos.y *= (1.0f - smooth);
					pPos.z *= (1.0f - smooth);
					player_->GetWorldTransform().translation_ = pPos;
					player_->GetWorldTransform().rotation_ = { 0.0f, 0.0f, 0.0f };
					player_->Update(MakeIdentity4x4());
				}

				// カメラをボスの位置から正面）へ滑らかに移動
				viewProjection_.translation_.x = warpCamStartPos_.x + (0.0f - warpCamStartPos_.x) * smooth;
				viewProjection_.translation_.y = warpCamStartPos_.y + (2.0f - warpCamStartPos_.y) * smooth;
				viewProjection_.translation_.z = warpCamStartPos_.z + (-20.0f - warpCamStartPos_.z) * smooth;
				// 回転も正面へ
				viewProjection_.rotation_.x = warpCamStartRot_.x * (1.0f - smooth);
				viewProjection_.rotation_.y = warpCamStartRot_.y * (1.0f - smooth);
				viewProjection_.rotation_.z = warpCamStartRot_.z * (1.0f - smooth);

				currentWarpIntensity = 15.0f;
			}
			else if (t < 3.0f) {
				float ease = (t - 1.0f) / 2.0f;
				float pZ = std::pow(ease, 3.0f) * 80.0f;

				if (player_) {
					player_->GetWorldTransform().translation_ = { 0.0f, 0.0f, pZ };
					player_->Update(MakeIdentity4x4());

					float beamLen = 50.0f + (ease * 100.0f);
					warpLaserL_.scale_ = { beamLen, 0.3f, 0.3f };
					warpLaserL_.rotation_ = { 0.0f, 1.5708f, 0.0f };
					warpLaserL_.translation_ = { -4.5f, 0.0f, pZ - (beamLen / 2.0f) };
					warpLaserL_.matWorld_ = MakeAffineMatrix(warpLaserL_.scale_, warpLaserL_.rotation_, warpLaserL_.translation_);
					warpLaserL_.TransferMatrix();

					warpLaserR_.scale_ = { beamLen, 0.3f, 0.3f };
					warpLaserR_.rotation_ = { 0.0f, 1.5708f, 0.0f };
					warpLaserR_.translation_ = { 4.5f, 0.0f, pZ - (beamLen / 2.0f) };
					warpLaserR_.matWorld_ = MakeAffineMatrix(warpLaserR_.scale_, warpLaserR_.rotation_, warpLaserR_.translation_);
					warpLaserR_.TransferMatrix();
				}
				viewProjection_.translation_ = { 0.0f, 2.0f, -20.0f };
				viewProjection_.rotation_ = { 0.0f, 0.0f, 0.0f }; // 完全に正面
				currentWarpIntensity = 20.0f + (ease * 30.0f);
			}
			else if (t < 4.5f) {
				float ease = (t - 3.0f) / 1.5f;
				float zoomEase = std::pow(ease, 5.0f); // 5乗の超急カーブ

				float pZ = 80.0f + (ease * 120.0f);
				float cZ = -20.0f + (zoomEase * (pZ - 15.0f - (-20.0f)));

				if (player_) {
					player_->GetWorldTransform().translation_ = { 0.0f, 0.0f, pZ };
					player_->Update(MakeIdentity4x4());

					float beamLen = 150.0f + (zoomEase * 600.0f);
					warpLaserL_.scale_ = { beamLen, 0.3f, 0.3f };
					warpLaserL_.translation_ = { -4.5f, 0.0f, pZ - (beamLen / 2.0f) };
					warpLaserL_.matWorld_ = MakeAffineMatrix(warpLaserL_.scale_, warpLaserL_.rotation_, warpLaserL_.translation_);
					warpLaserL_.TransferMatrix();

					warpLaserR_.scale_ = { beamLen, 0.3f, 0.3f };
					warpLaserR_.translation_ = { 4.5f, 0.0f, pZ - (beamLen / 2.0f) };
					warpLaserR_.matWorld_ = MakeAffineMatrix(warpLaserR_.scale_, warpLaserR_.rotation_, warpLaserR_.translation_);
					warpLaserR_.TransferMatrix();
				}

				Vector3 camPos = { 0.0f, 2.0f, cZ };
				float shake = ease * 0.05f;
				camPos.x += (std::rand() % 10 - 5) * shake;
				camPos.y += (std::rand() % 10 - 5) * shake;
				viewProjection_.translation_ = camPos;

				currentWarpIntensity = 50.0f + (zoomEase * 300.0f);
			}
			//超加速
			else if (t < 5.0f) {
				float ease = (t - 4.5f) / 0.5f;
				float pZ = 200.0f + (std::pow(ease, 3.0f) * 1000.0f);

				if (player_) {
					player_->GetWorldTransform().translation_ = { 0.0f, 0.0f, pZ };
					player_->Update(MakeIdentity4x4());

					float beamLen = 800.0f;
					warpLaserL_.scale_ = { beamLen, 0.3f, 0.3f };
					warpLaserL_.translation_ = { -4.5f, 0.0f, pZ - (beamLen / 2.0f) };
					warpLaserL_.matWorld_ = MakeAffineMatrix(warpLaserL_.scale_, warpLaserL_.rotation_, warpLaserL_.translation_);
					warpLaserL_.TransferMatrix();

					warpLaserR_.scale_ = { beamLen, 0.3f, 0.3f };
					warpLaserR_.translation_ = { 4.5f, 0.0f, pZ - (beamLen / 2.0f) };
					warpLaserR_.matWorld_ = MakeAffineMatrix(warpLaserR_.scale_, warpLaserR_.rotation_, warpLaserR_.translation_);
					warpLaserR_.TransferMatrix();
				}
				viewProjection_.translation_ = { 0.0f, 2.0f, 185.0f };
				currentWarpIntensity = 350.0f + (ease * 400.0f);
			}
			else {
				// 自機やレーザーを画面外へ退避
				if (player_) {
					player_->GetWorldTransform().translation_.z = 9999.0f;
					player_->Update(MakeIdentity4x4());
				}
				warpLaserL_.translation_.z = 9999.0f; warpLaserL_.TransferMatrix();
				warpLaserR_.translation_.z = 9999.0f; warpLaserR_.TransferMatrix();

				warpStarTf_.translation_.z = 9999.0f; warpStarTf_.TransferMatrix();
				currentWarpIntensity = 0.0f;

				if (warpVoice_) {
					Audio::GetInstance()->SoundStopWave(warpVoice_);
					warpVoice_ = nullptr;
				}

				if (!resultUI_->IsActive()) {
					resultUI_->Start(totalScore_, playTime_);
				}

				// 前回実装した退出アニメーションの処理はそのまま残す
				if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
					if (!resultUI_->IsExiting()) {
						resultUI_->StartExit();
					}
				}

				// 退出アニメーションが完全に終わった瞬間、シーンをタイトルに切り替える
				if (resultUI_->IsExitFinished()) {
					resultUI_->Stop();

					if (titleUI_) titleUI_->Reset();

					totalScore_ = 0;
					scoreAtWaveStart_ = 0;
					playTime_ = 0.0f;

					if (waveManager_) waveManager_->Initialize();

					if (scoreUI_) scoreUI_->SetScore(0);
					if (lifeUI_) lifeUI_->SetLife(5);
					if (rail_) rail_->Initialize();
					if (railCamera_) railCamera_->Initialize(rail_);
					if (player_) player_->SetDead(false);
					for (BaseEnemy* enemy : enemies_) delete enemy;
					enemies_.clear();
					for (EnemyBullet* bullet : enemyBullets_) delete bullet;
					enemyBullets_.clear();

					sceneState_ = SceneState::Title;
					sceneTimer_ = 0.0f;

					if (bgmVoice_ == nullptr) {
						bgmVoice_ = Audio::GetInstance()->SoundPlayWave(bgmSound_, true);
					}
				}
			}
		}
else if (sceneState_ == SceneState::Rebooting) {
	if (rebootUI_) {
		rebootUI_->Update();

		if (rebootUI_->IsFinished()) {
			// 今のウェーブを最初からやり直す
			if (waveManager_) waveManager_->RestartCurrentWave(enemies_, enemyBullets_);

			// 残機を復活させる
			if (lifeUI_) lifeUI_->SetLife(5);

			totalScore_ = scoreAtWaveStart_;
			if (scoreUI_) scoreUI_->SetScore(totalScore_);

			// プレイヤーを復活させ、定位置に戻す
			if (player_) {
				player_->SetDead(false);

				player_->SetInvincible(180);
				player_->GetWorldTransform().translation_ = { 0.0f, -2.0f, 15.0f }; // 初期位置
				player_->Update(MakeIdentity4x4());
			}

			// 再びワープエフェクトから勢いよくスタート
			sceneState_ = SceneState::StartWarp;
			sceneTimer_ = 0.0f;
			warpCamStartPos_ = viewProjection_.translation_;

			if (bgmVoice_) {
				Audio::GetInstance()->SoundStopWave(bgmVoice_);
				bgmVoice_ = nullptr;
			}

			warpVoice_ = Audio::GetInstance()->SoundPlayWave(warpSound_);
		}
	}
		}

		viewProjection_.UpdateMatrix();
		warpEffect_->Update(currentWarpIntensity, warpParent);

		if (skydome_) skydome_->Update();
		if (groundModel_) {
			groundModel_->GetTransform().scale = groundTransform_.scale_;
			groundModel_->GetTransform().rotate = groundTransform_.rotation_;
			groundModel_->GetTransform().translate = groundTransform_.translation_;
			groundModel_->SetCameraMatrix(viewProjection_.matView, viewProjection_.matProjection);
			groundModel_->Update();
		}


		if (myNeonBar_ && player_) {
			Vector3 camPos = viewProjection_.translation_;
			myNeonBar_->Update(camPos);

			Vector3 neonPos = myNeonBar_->GetPosition();
			Vector3 nColor = myNeonBar_->GetNeonColor();
			float nIntensity = myNeonBar_->GetIntensity();
			player_->SetPointLight(neonPos, nColor, nIntensity, 30.0f, camPos);

			NeonModel::DirectionalLight* lightData = myNeonBar_->GetModel()->GetLightData();
			if (lightData) {
				lightData->pointPos = player_->GetWorldPosition();
				lightData->pointColor = { 0.0f, 1.0f, 0.5f, 1.0f }; // インゲームのネオン色
				lightData->pointIntensity = 5.0f;
				lightData->pointRadius = 12.0f;
			}
		}


		return;
	}
	/*-----------------------
	ワープエフェクト
	---------------------------*/
	float targetWarpIntensity = 0.0f;

	// ウェーブ間のインターバル中なら
	if (waveManager_ && !waveManager_->IsWaveActive()) {
		targetWarpIntensity = 15.0f;
	}


	Matrix4x4 warpParentMat = railCamera_ ? railCamera_->GetWorldMatrix() : MakeIdentity4x4();

	warpEffect_->Update(targetWarpIntensity, warpParentMat);

	/*-------------------------
	レールカメラ
	---------------------------*/
	if (railCamera_) {
		railCamera_->Update();
	}

	// エディター
	if (isDebugCameraActive_ && debugCamera_ != nullptr) {
		debugCamera_->Update();
		viewProjection_.matView = debugCamera_->GetViewMatrix();
		viewProjection_.matProjection = debugCamera_->GetProjectionMatrix();

		// レールエディターを動かす
		if (railEditor_) railEditor_->Update(debugCamera_, railCamera_);
	}

	if (groundModel_) {
		groundModel_->GetTransform().scale = groundTransform_.scale_;
		groundModel_->GetTransform().rotate = groundTransform_.rotation_;
		groundModel_->GetTransform().translate = groundTransform_.translation_;
		groundModel_->SetCameraMatrix(viewProjection_.matView, viewProjection_.matProjection);
		groundModel_->Update();
	}



	/*-----------------------------
	エネミー更新
	--------------------------------*/
	for (BaseEnemy* enemy : enemies_) {
		enemy->Update();
	}

	// デスフラグが立った敵をリストから除外してメモリ解放
	enemies_.remove_if([this](BaseEnemy* enemy) {
		if (enemy->IsDead()) {
			particleManager_->Emit(enemy->GetTranslation(), 60, { 1.0f, 0.0f, 0.8f });
			totalScore_ += 100;
			if (scoreUI_) scoreUI_->AddScore(100);
			if (cameraShake_) {
				cameraShake_->Start(1.5f, 10);
			}
			delete enemy;
			return true;
		}
		return false;
		});

	for (EnemyBullet* bullet : enemyBullets_) {
		bullet->Update();
	}

	// デスフラグが立った敵弾をリストから除外してメモリ解放
	enemyBullets_.remove_if([](EnemyBullet* bullet) {
		if (bullet->IsDead()) {
			delete bullet;
			return true;
		}
		return false;
		});

	/*-----------------------------
プレイヤー更新
--------------------------------*/
	if (player_) {
		Matrix4x4 parentMat = railCamera_ ? railCamera_->GetWorldMatrix() : MakeIdentity4x4();
		player_->Update(parentMat);
	}

	//waveManagerの更新
	waveManager_->Update(enemies_, player_, this);

	if (waveManager_) {
		bool isWaveActive = waveManager_->IsWaveActive();
		if (isWaveActive && !wasWaveActive_) {
			scoreAtWaveStart_ = totalScore_; // 今のスコアを記録！

			if (warpVoice_) {
				Audio::GetInstance()->SoundStopWave(warpVoice_);
				warpVoice_ = nullptr;
			}
		}
		else if (!isWaveActive && wasWaveActive_) {
			warpVoice_ = Audio::GetInstance()->SoundPlayWave(warpSound_);
		}
		wasWaveActive_ = isWaveActive;
	}

	/*-------------------------------
	天球
	----------------------------------*/
	skydome_->Update();

	// 軸表示
	AxisIndicator::GetInstance()->Update();

	/*------------------
	パーティクル
	------------------------------*/
	particleManager_->Update();

	/*------------------------------
	カメラシェイク
	----------------------------*/
	cameraShake_->Update();

	/*-------------------------
	UI表示
	------------------------------*/
	WarningUI::GetInstance()->Update();
	if (scoreUI_) {
		scoreUI_->Update();
	}


#ifdef _DEBUG 
	if (Input::GetInstance()->TriggerKey(DIK_P)) {
		isDebugCameraActive_ = !isDebugCameraActive_;
	}

	if (isDebugCameraActive_ && debugCamera_ != nullptr) {
		debugCamera_->Update();
		viewProjection_.matView = debugCamera_->GetViewMatrix();
		viewProjection_.matProjection = debugCamera_->GetProjectionMatrix();

		viewProjection_.translation_ = debugCamera_->GetTranslation();
		viewProjection_.rotation_ = debugCamera_->GetRotation();
		viewProjection_.UpdateMatrix();
	}
	else {
		// 通常のカメラ更新
		if (railCamera_) {
			viewProjection_ = railCamera_->GetViewProjection();
		}
		else {
			viewProjection_.UpdateMatrix();
		}
	}
#else 
	if (railCamera_) {
		viewProjection_ = railCamera_->GetViewProjection();
	}
#endif

	bool isBossDying = false;
	Vector3 bossPos = { 0,0,0 };
	float dyingTime = 0.0f;

	// 敵リストの中に「死にかけのボス」がいるか探す
	for (BaseEnemy* enemy : enemies_) {
		EnemyBoss* boss = dynamic_cast<EnemyBoss*>(enemy);


		if (boss && boss->IsDying()) {
			isBossDying = true;
			bossPos = boss->GetWorldPosition();
			dyingTime = boss->GetAnimeTime();
			break;
		}
	}

	if (isBossDying) {
		float angle = dyingTime * 0.2f;
		float distance = 130.0f - (dyingTime * 2.0f);
		if (distance < 100.0f) distance = 100.0f;

		float height = 40.0f;

		Vector3 camPos = {
			bossPos.x + std::sin(angle) * distance,
			bossPos.y + height,
			bossPos.z + std::cos(angle) * distance
		};

		// カメラからボスへのベクトルを計算して注視
		Vector3 forward = { bossPos.x - camPos.x, bossPos.y - camPos.y, bossPos.z - camPos.z };
		float xzLength = std::sqrt(forward.x * forward.x + forward.z * forward.z);

		viewProjection_.translation_ = camPos;
		viewProjection_.rotation_.y = std::atan2(forward.x, forward.z);
		viewProjection_.rotation_.x = std::atan2(-forward.y, xzLength);
		viewProjection_.rotation_.z = 0.0f; 

		// 揺れも最低限にして見やすくする
		float shakeScale = dyingTime * 0.015f;

		if (shakeScale > 0.15f) {
			shakeScale = 0.15f;
		}
		viewProjection_.translation_.x += (std::rand() % 10 - 5) * shakeScale;
		viewProjection_.translation_.y += (std::rand() % 10 - 5) * shakeScale;

		viewProjection_.UpdateMatrix();
		if (dyingTime >= 18.0f) {
			sceneState_ = SceneState::ClearWarp;
			sceneTimer_ = 0.0f;


			// 現在のカメラ座標と角度を保存しておく
			warpCamStartPos_ = viewProjection_.translation_;
			warpCamStartRot_ = viewProjection_.rotation_;

			// ボスをリストから除外して消滅させる
			enemies_.remove_if([](BaseEnemy* enemy) {
				if (dynamic_cast<EnemyBoss*>(enemy)) {
					delete enemy;
					return true;
				}
				return false;
				});

			for (EnemyBullet* bullet : enemyBullets_) {
				delete bullet;
			}
			enemyBullets_.clear();

			warpVoice_ = Audio::GetInstance()->SoundPlayWave(warpSound_);
		}
	}

	else if (player_) {
		float targetCameraRoll = player_->GetRotation().z * 0.4f;
		cameraRoll_ += (targetCameraRoll - cameraRoll_) * 0.1f;
		viewProjection_.rotation_.z = cameraRoll_;

		Vector3 shakeOffset = cameraShake_->GetOffset();
		viewProjection_.translation_.x += shakeOffset.x;
		viewProjection_.translation_.y += shakeOffset.y;
		viewProjection_.translation_.z += shakeOffset.z;

		viewProjection_.UpdateMatrix();
	}

	// ==========================================
	// カメラとネオンの連動処理
	// ==========================================
	Vector3 camPos = viewProjection_.translation_;
	if (isDebugCameraActive_ && debugCamera_ != nullptr) {
		Matrix4x4 v = debugCamera_->GetViewMatrix();
		camPos.x = -(v.m[3][0] * v.m[0][0] + v.m[3][1] * v.m[1][0] + v.m[3][2] * v.m[2][0]);
		camPos.y = -(v.m[3][0] * v.m[0][1] + v.m[3][1] * v.m[1][1] + v.m[3][2] * v.m[2][1]);
		camPos.z = -(v.m[3][0] * v.m[0][2] + v.m[3][1] * v.m[1][2] + v.m[3][2] * v.m[2][2]);
	}
	else {
		viewProjection_.UpdateMatrix();
	}

	// ---------------------------------
	// ネオンバーの更新
	if (myNeonBar_ != nullptr) {
		myNeonBar_->Update(camPos);

		if (player_ != nullptr) {
	
			if (sceneState_ == SceneState::Title || sceneState_ == SceneState::StartWarp) {

				player_->SetPointLight(camPos, { 1.0f, 1.0f, 1.0f }, 100.0f, 100.0f, camPos);
			}
			else {
				// 通常のインゲーム中はこちらの処理
				Vector3 neonPos = myNeonBar_->GetPosition();
				Vector3 nColor = myNeonBar_->GetNeonColor();
				float nIntensity = myNeonBar_->GetIntensity();
				player_->SetPointLight(neonPos, nColor, nIntensity, 30.0f, camPos);
			}
		}

		NeonModel::DirectionalLight* lightData = myNeonBar_->GetModel()->GetLightData();
		if (lightData != nullptr && player_ != nullptr) {
			lightData->pointPos = player_->GetWorldPosition();
			lightData->pointColor = { 0.0f, 1.0f, 0.5f, 1.0f };
			lightData->pointIntensity = 5.0f;
			lightData->pointRadius = 12.0f;
		}
	}

	

	// ==========================================
	//  当たり判定処理
	// ==========================================
	collisionManager_->ClearColliders();

	if (player_) {
		collisionManager_->AddCollider(player_);
		for (PlayerBullet* pBullet : player_->GetBullets()) {
			if (!pBullet->IsDead()) collisionManager_->AddCollider(pBullet);
		}
	}

	for (BaseEnemy* enemy : enemies_) {
		collisionManager_->AddCollider(enemy);
	}
	for (EnemyBullet* eBullet : enemyBullets_) {
		collisionManager_->AddCollider(eBullet);
	}

	collisionManager_->CheckAllCollisions();

	if (sceneState_ != SceneState::Title) {
		float camZ = viewProjection_.translation_.z;
		for (int i = 0; i < kMaxAmbientParticles; ++i) {
			ambientParticles_[i].position.z -= ambientParticles_[i].zSpeed;
			ambientParticles_[i].rotation.x += ambientParticles_[i].rotSpeed.x;
			ambientParticles_[i].rotation.y += ambientParticles_[i].rotSpeed.y;
			ambientParticles_[i].rotation.z += ambientParticles_[i].rotSpeed.z;

			if (ambientParticles_[i].position.z < camZ + 40.0f) {
				float spawnX = (std::rand() % 200 - 100) * 1.0f;
				float spawnY = (std::rand() % 120 - 60) * 1.0f;

				// 再配置時も真ん中を避ける
				if (spawnX > -40.0f && spawnX < 40.0f) spawnX = (spawnX > 0) ? spawnX + 40.0f : spawnX - 40.0f;
				if (spawnY > -30.0f && spawnY < 30.0f) spawnY = (spawnY > 0) ? spawnY + 30.0f : spawnY - 30.0f;

				ambientParticles_[i].position.x = spawnX;
				ambientParticles_[i].position.y = spawnY;
				ambientParticles_[i].position.z = camZ + 200.0f + (std::rand() % 50);
			}

			ambientTransforms_[i].scale_ = { ambientParticles_[i].scale, ambientParticles_[i].scale, ambientParticles_[i].scale };
			ambientTransforms_[i].rotation_ = ambientParticles_[i].rotation;
			ambientTransforms_[i].translation_ = ambientParticles_[i].position;

			ambientTransforms_[i].matWorld_ = MakeAffineMatrix(
				ambientTransforms_[i].scale_,
				ambientTransforms_[i].rotation_,
				ambientTransforms_[i].translation_
			);
			ambientTransforms_[i].TransferMatrix();
		}
	}
}

void GameScene::Draw() {
	// ==========================================
	// 普通のやつ
	// ==========================================
	bool isBlackout = (sceneState_ == SceneState::ClearWarp && sceneTimer_ >= 5.0f);
		skydome_->Draw(viewProjection_);
	if (!isBlackout) {

		/*
		if (sceneState_ != SceneState::Title && sceneState_ != SceneState::StartWarp && sceneState_ != SceneState::ClearWarp) {
			AxisIndicator::GetInstance()->Draw();
		}
		*/

		if (groundModel_) {
			groundModel_->Draw(groundTransform_, viewProjection_, groundTex_);
		}
	}


	// ==========================================
	//  ネオン
	// ==========================================
	bloom_->PreDraw();
	if (warpStarModel_ && sceneState_ != SceneState::Title && sceneState_ != SceneState::StartWarp) {
		uint32_t whiteTex = TextureManager::Load("Resources/white.png");
		for (int i = 0; i < kMaxAmbientParticles; ++i) {
			warpStarModel_->SetNeonColor(
				ambientParticles_[i].intensity,
				ambientParticles_[i].color.x,
				ambientParticles_[i].color.y,
				ambientParticles_[i].color.z
			);
			warpStarModel_->Draw(ambientTransforms_[i], viewProjection_, whiteTex);
		}
	}

	if (!isBlackout) {
		warpEffect_->Draw(viewProjection_);
	}


	if (sceneState_ == SceneState::StartWarp) {
		uint32_t whiteTex = TextureManager::Load("Resources/white.png");
		if (warpLaserModel_) {
			warpLaserModel_->SetNeonColor(80.0f, 0.8f, 0.9f, 1.0f);
			warpLaserModel_->Draw(warpLaserL_, viewProjection_, whiteTex);
			warpLaserModel_->Draw(warpLaserR_, viewProjection_, whiteTex);
		}
	}

	// ワープ演出専用オブジェクトの描画
	if (sceneState_ == SceneState::ClearWarp) {
		uint32_t whiteTex = TextureManager::Load("Resources/white.png");

		// 光の線（1.0秒 ～ 5.0秒）
		if (sceneTimer_ >= 1.0f && sceneTimer_ < 5.0f) {
			if (warpLaserModel_) {
				warpLaserModel_->SetNeonColor(80.0f, 0.8f, 0.9f, 1.0f);
				warpLaserModel_->Draw(warpLaserL_, viewProjection_, whiteTex);
				warpLaserModel_->Draw(warpLaserR_, viewProjection_, whiteTex);
			}
		}
		
	}

	enemies_.sort([](BaseEnemy* a, BaseEnemy* b) {
		return a->GetWorldPosition().z > b->GetWorldPosition().z;
		});

	// 通常の敵・弾の描画
	if (sceneState_ != SceneState::ClearWarp) {
		for (BaseEnemy* enemy : enemies_) {
			enemy->DrawNeon(viewProjection_);
		}
		for (EnemyBullet* bullet : enemyBullets_) {
			bullet->Draw(viewProjection_);
		}
	}


	//  自機の描画
	if (player_ && !(sceneState_ == SceneState::ClearWarp && sceneTimer_ >= 5.0f)) {
		player_->DrawNeon(viewProjection_);  // 光るパーツ
		player_->Draw(viewProjection_);      // 暗いパーツ
	}

	for (BaseEnemy* enemy : enemies_) {
		enemy->DrawNeon(viewProjection_);
	}

	for (BaseEnemy* enemy : enemies_) {
		enemy->DrawNeon(viewProjection_);
	}

	for (BaseEnemy* enemy : enemies_) {
		enemy->DrawNeon(viewProjection_);
	}

	for (BaseEnemy* enemy : enemies_) {
		enemy->DrawNeon(viewProjection_);
	}



	for (BaseEnemy* enemy : enemies_) {
		enemy->DrawNeon(viewProjection_);
	}

	for (EnemyBullet* bullet : enemyBullets_) {
		bullet->Draw(viewProjection_);
	}

	if (resultUI_ && resultUI_->IsActive()) {
		resultUI_->Draw(viewProjection_);
	}

	// ロックオンUI
	if (player_) {
		player_->DrawUI(viewProjection_);
	}

	// パーティクル
	particleManager_->Draw(viewProjection_);




	/*---------------------
	UI表示
	-------------------------*/
	if (titleUI_ && titleUI_->IsActive()) {
		titleUI_->Draw();
	}

	// インゲーム中のUI
	if (sceneState_ == SceneState::Playing) {
		if (scoreUI_) scoreUI_->Draw();
		if (lifeUI_) lifeUI_->Draw();
		if (waveUI_) waveUI_->Draw();
		if (tutorialUI_) {
			tutorialUI_->DrawBase(); 
			tutorialUI_->DrawNeon(); 
		}
	}

	// リブート画面のUI
	if (sceneState_ == SceneState::Rebooting) {
		if (rebootUI_) rebootUI_->Draw();
	}

	// リザルト画面のUI
	if (sceneState_ == SceneState::ClearWarp && sceneTimer_ >= 5.0f) {
		if (resultUI_ && resultUI_->IsActive()) {
			resultUI_->Draw(viewProjection_);
		}
	}

	// ボスの警告UI
	WarningUI::GetInstance()->Draw();

	// HDRキャンバスへの書き込み終了、普通の画面に戻る
	bloom_->PostDraw();

	bloom_->Execute();
	bloom_->DrawResult();

}