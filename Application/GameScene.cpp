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

	// ネオン
	neonText_Open_ = new NeonText();
	neonText_Open_->Initialize(dxCommon_);
	neonText_Open_->Print("OPEN", -3.0f, 0.0f, 0.8f);

	// 枠線用
	neonText_Border_ = new NeonText();
	neonText_Border_->Initialize(dxCommon_);
	neonText_Border_->Print("O", -0.9f, 0.0f, 3.0f);

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

	// editor 
	railEditor_ = new RailEditor();
	railEditor_->Initialize(rail_);

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

	/*------------------------
	パーティクル
	--------------------------*/
	particleModel_ = new NeonModel();
	particleModel_->Initialize("Resources", "block.obj");

	particleManager_ = new ParticleManager();
	particleManager_->Initialize(particleModel_, TextureManager::Load("Resources/white.png"));

}

void GameScene::Update() {

	/*-----------------------
	ワープエフェクト
	---------------------------*/
	float targetWarpIntensity = 0.0f;

	// ウェーブ間のインターバル中なら
	if (waveManager_ && !waveManager_->IsWaveActive()) {
		// 目標スピードを跳ね上げる 
		targetWarpIntensity = 15.0f;
	}

	warpEffect_->Update(targetWarpIntensity);

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
	プレイヤー更新
	--------------------------------*/
	if (player_) {
		Matrix4x4 parentMat = railCamera_ ? railCamera_->GetWorldMatrix() : MakeIdentity4x4();
		player_->Update(parentMat);
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

	//waveManagerの更新
	waveManager_->Update(enemies_, player_, this);

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

	if (player_) {
		
		float targetCameraRoll = player_->GetRotation().z * 0.4f;

		// 滑らかに目標の傾きへ近づける
		cameraRoll_ += (targetCameraRoll - cameraRoll_) * 0.1f;

		// ビュー行列にZ回転を足す
		viewProjection_.rotation_.z = cameraRoll_;

		// 傾きを反映した上で、最終的な行列を更新！
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
			Vector3 neonPos = myNeonBar_->GetPosition();
			Vector3 nColor = myNeonBar_->GetNeonColor();
			float nIntensity = myNeonBar_->GetIntensity();
			player_->SetPointLight(neonPos, nColor, nIntensity, 30.0f, camPos);
		}

		NeonModel::DirectionalLight* lightData = myNeonBar_->GetModel()->GetLightData();
		if (lightData != nullptr && player_ != nullptr) {
			lightData->pointPos = player_->GetWorldPosition();
			lightData->pointColor = { 0.0f, 1.0f, 0.5f, 1.0f };
			lightData->pointIntensity = 5.0f;
			lightData->pointRadius = 12.0f;
		}
	}

	// ---------------------------------
	// ネオン文字
	float currentTextIntensity = neonTextFlicker_.GetIntensity(neonIntensity_);

	if (neonText_Open_ != nullptr) {
		neonText_Open_->SetMaterial(neonRadius_, neonSoftness_, currentTextIntensity, neonColor_[0], neonColor_[1], neonColor_[2], neonLengthOffset_);
		if (isDebugCameraActive_ && debugCamera_ != nullptr) {
			neonText_Open_->Update(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
		}
		else {
			neonText_Open_->Update(viewProjection_.matView, viewProjection_.matProjection);
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

	// WaveManager
	waveManager_->DrawImGui();

	ImGui::End(); // Master Controlの終了

#endif
}

void GameScene::Draw() {
	// ==========================================
	// 普通のやつ
	// ==========================================
	AxisIndicator::GetInstance()->Draw();
	skydome_->Draw(viewProjection_);

	if (groundModel_) {
		groundModel_->Draw(groundTransform_, viewProjection_, groundTex_);
	}

	// ==========================================
	//  ネオン
	// ==========================================
	bloom_->PreDraw();

	warpEffect_->Draw(viewProjection_);

	if (player_) {

				player_->DrawNeon(viewProjection_);  // 光るパーツ
				player_->Draw(viewProjection_);      // 暗いパーツ

	}

	for (BaseEnemy* enemy : enemies_) { 
		enemy->DrawNeon(viewProjection_);
	}

	for (EnemyBullet* bullet : enemyBullets_) {
		bullet->Draw(viewProjection_);
	}

	// パーティクル
	particleManager_->Draw(viewProjection_);

	// HDRキャンバスへの書き込み終了、普通の画面に戻る
	bloom_->PostDraw();

	bloom_->Execute();
	bloom_->DrawResult();
}