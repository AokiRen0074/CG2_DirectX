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

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif



		





GameScene::~GameScene() {
	delete debugCamera_;
	delete player_;
	delete enemy_;
	delete collisionManager_;
	delete skydomeModel_;
	delete skydome_;
	//delete bulletModel_;
	delete groundModel_;
}

void GameScene::Initialize(DirectXCommon* dxCommon) {

	dxCommon_ = dxCommon;
	
	//
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

	// プレイヤー

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
	bloom_->Initialize(dxCommon_, 1280,720);
	


	// エネミー

	enemyObject_->Initialize("Resources", "block.obj");
	enemyTex_ = TextureManager::Load("Resources/monsterBall.png");



	/*----------------------
	スプライトの生成と初期化
	-------------------------*/

	/*-----------------------
	天球の生成と初期化
	-----------------------------*/
	// 天球モデル
	skydomeModel_ = new Object3d();
	skydomeModel_->Initialize("Resources/skyDome", "AL3_skyDome.obj");

	// 天球のテクスチャ
	skydomeTex_ = TextureManager::Load("Resources/skyDome/AL3_skydome.png");

	skydome_ = new Skydome();
	skydome_->Initialize(skydomeModel_, skydomeTex_);

	/*--------------------------------
	エディターパネルの描画
	-------------------------------*/


	/*-------------------------------
	自キャラ生成と初期化
	----------------------------------*/
	Player::RegisterGlobalVariables();
	// 自キャラの生成
	player_ = new Player();

	// 自キャラの初期化
	player_->Initialize();


	// 敵キャラの生成
	enemy_ = new Enemy();

	// 敵キャラに自キャラのアドレスを渡す
	enemy_->SetPlayer(player_);

	// 敵キャラの生成


	enemy_ = new Enemy();
	enemy_->Initialize(player_);



	/*-------------------------
	衝突マネージャー
	------------------------------*/
	collisionManager_ = new CollisionManager();



	/*-----------------------
	軸表示
	------------------------*/
	AxisIndicator::GetInstance()->Initialize();

	// 軸方向の表示を有効にする
	AxisIndicator::GetInstance()->SetVisible(true);

	// 軸方向表示が参照するビュープロジェクションの指定
	AxisIndicator::GetInstance()->SetTargetCamera(&viewProjection_);

}



void GameScene::Update() {

	/*-----------------------------
	プレイヤー更新
	--------------------------------*/
	player_->Update();

	/*-----------------------------
		エネミー更新
--------------------------------*/
	if (enemy_) enemy_->Update();

	/*-------------------------------
	天球
	----------------------------------*/
	skydome_->Update();




	// 軸表示
	AxisIndicator::GetInstance()->Update();

#ifdef _DEBUG 
	if (Input::GetInstance()->TriggerKey(DIK_P)) {
		isDebugCameraActive_ = !isDebugCameraActive_;
	}

	if (isDebugCameraActive_ && debugCamera_ != nullptr) {
		debugCamera_->Update();
		viewProjection_.matView = debugCamera_->GetViewMatrix();
		viewProjection_.matProjection = debugCamera_->GetProjectionMatrix();
	}
	else {
		// 通常のカメラ更新
		viewProjection_.UpdateMatrix();
	}
#endif

	// ==========================================
	// カメラとネオンの連動処理
	// ==========================================
	Vector3 camPos = viewProjection_.translation_;
	if (isDebugCameraActive_ && debugCamera_ != nullptr) {
		// ビュー行列からカメラのワールド座標を逆算
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

		// ネオンの光を自機(Player)へ送る
		if (player_ != nullptr) {
			Vector3 neonPos = myNeonBar_->GetPosition();
			Vector3 nColor = myNeonBar_->GetNeonColor();
			float nIntensity = myNeonBar_->GetIntensity();
			player_->SetPointLight(neonPos, nColor, nIntensity, 30.0f, camPos);
		}

		// ネオンバー自体が発する点光源の設定
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
	if (enemy_) {
		collisionManager_->AddCollider(enemy_);
		for (EnemyBullet* eBullet : enemy_->GetBullets()) {
			if (!eBullet->IsDead()) collisionManager_->AddCollider(eBullet);
		}
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
		if (enemy_) enemy_->DrawImGui();
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

	/*
	if (enemy_) {
		enemy_->Draw(viewProjection_);
	}
	*/


	// 暗いパーツ
	//player_->Draw(viewProjection_);


	// ==========================================
	//  ネオン
	// ==========================================
	bloom_->PreDraw();

	
	if (player_) {
		player_->Draw(viewProjection_);      // 暗いパーツ
		player_->DrawNeon(viewProjection_);  // 光るパーツ
	}
	
	if (enemy_) enemy_->DrawNeon(viewProjection_);
	

	
	//if (neonText_Border_ != nullptr) { neonText_Border_->Draw(); }
	//if (neonText_Open_ != nullptr) { neonText_Open_->Draw(); }

	
	/*
	if (myNeonBar_ != nullptr) {
		myNeonBar_->Draw(viewProjection_);
	}
	*/
	
	


	// HDRキャンバスへの書き込み終了、普通の画面(R8)に戻る
	bloom_->PostDraw();


	bloom_->Execute();
	bloom_->DrawResult();
}




