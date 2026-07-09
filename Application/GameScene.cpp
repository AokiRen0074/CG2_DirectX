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



	bloom_ = new Bloom();
	// 画面サイズ（1280x720）を渡す
	bloom_->Initialize(dxCommon_, WindowApp::kClientWidth, WindowApp::kClientHeight);
	


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
	enemy_->Initialize(enemyObject_, enemyTex_);



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

	static float neonRadius = 0.03f;
	static float neonSoftness = 15.0f;
	static float neonIntensity = 8.0f;
	static float neonColor[3] = { 0.0f, 0.8f, 1.0f };
	static float neonLengthOffset = -0.2f;

#ifdef USE_IMGUI

	static Vector3 startPos = { -4.0f, 2.0f, 0.0f };
	static Vector3 endPos = { 4.0f, 2.0f, 0.0f };
	static float canvasThickness = 1.0f;
	ImGui::Begin("Procedural Neon");
	ImGui::DragFloat3("Start Pos", &startPos.x, 0.1f);
	ImGui::DragFloat3("End Pos", &endPos.x, 0.1f);
	ImGui::SliderFloat("Canvas Thickness", &canvasThickness, 0.1f, 5.0f);
	ImGui::End();
#endif

	/*-------------------------
	デバッグカメラ
	--------------------------*/
#ifdef _DEBUG 
	if (Input::GetInstance()->TriggerKey(DIK_P)) {
		isDebugCameraActive_ = !isDebugCameraActive_;
	}


	if (isDebugCameraActive_) {
		debugCamera_->Update();

		viewProjection_.matView = debugCamera_->GetViewMatrix();
		viewProjection_.matProjection = debugCamera_->GetProjectionMatrix();
	}
	else {
		viewProjection_.UpdateMatrix();
	}
#endif


	/*------------------
	自キャラ更新
	----------------------*/

	player_->Update();



	/*------------------
	敵キャラ更新
	------------------*/
	if (enemy_) {
		enemy_->Update();
	}

	/*--------------------
	天球
	---------------------------*/
	skydome_->Update();

	// オブジェクトの更新
	if (isDebugCameraActive_ && debugCamera_ != nullptr) {
		debugCamera_->Update();

		// ここで安全に取得する
		//object3d_->SetCameraMatrix(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
	}


	if (myNeonBar_ != nullptr) {
		myNeonBar_->Update();
		myNeonBar_->DrawImGui("Neon Bar Test"); // ここでImGuiのウィンドウを描画！
	}


	static float time = 0.0f;
	time += 1.0f / 60.0f;
	float flickerIntensity = neonIntensity;

	if (sinf(time * 12.0f) > 0.7f) {
		flickerIntensity *= (0.2f + (rand() % 100 / 100.0f) * 0.8f);
	}
	if (rand() % 1000 < 10) { flickerIntensity = 0.0f; }

	if (neonText_Open_ != nullptr) {
		neonText_Open_->SetMaterial(neonRadius, neonSoftness, flickerIntensity, 1.0f, 0.2f, 0.2f, neonLengthOffset);
		if (isDebugCameraActive_ && debugCamera_ != nullptr) {
			neonText_Open_->Update(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
		}
		else {
			neonText_Open_->Update(viewProjection_.matView, viewProjection_.matProjection);
		}
	}


	AxisIndicator::GetInstance()->Update();

	/*-----------------------
	当たり判定処理
	-------------------------*/
	collisionManager_->ClearColliders();

	// コライダーを全て衝突マネージャのリストに登録する
	if (player_) {
		collisionManager_->AddCollider(player_);

		// 自弾を登録
		const std::list<PlayerBullet*>& playerBullets = player_->GetBullets();
		for (PlayerBullet* pBullet : playerBullets) {
			if (!pBullet->IsDead()) {
				collisionManager_->AddCollider(pBullet);
			}
		}
	}

	if (enemy_) {
		collisionManager_->AddCollider(enemy_);

		// 敵弾を登録
		const std::list<EnemyBullet*>& enemyBullets = enemy_->GetBullets();
		for (EnemyBullet* eBullet : enemyBullets) {
			if (!eBullet->IsDead()) {
				collisionManager_->AddCollider(eBullet);
			}
		}
	}

	// 衝突マネージャの当たり判定処理を呼び出す
	collisionManager_->CheckAllCollisions();



#ifdef USE_IMGUI
	ImGui::ShowDemoWindow();
	GlobalVariables::GetInstance()->Update();
#endif
}

void GameScene::Draw() {
	// ==========================================
	// 普通のやつ
	// ==========================================
	AxisIndicator::GetInstance()->Draw();
	skydome_->Draw(viewProjection_);
	if (enemy_) {
		enemy_->Draw(viewProjection_);
	}

	// 「暗いパーツ」をここで描画
	//player_->Draw(viewProjection_);


	// ==========================================
	//  ネオン
	// ==========================================
	bloom_->PreDraw();

	// 自機の「光るパーツ」と「ネオン文字」だけをここで描画！
//player_->DrawNeon(viewProjection_);

	
	//if (neonText_Border_ != nullptr) { neonText_Border_->Draw(); }
	//if (neonText_Open_ != nullptr) { neonText_Open_->Draw(); }

	
	if (myNeonBar_ != nullptr) {
		myNeonBar_->Draw(viewProjection_);
	}


	// HDRキャンバスへの書き込み終了、普通の画面(R8)に戻る
	bloom_->PostDraw();

	// ==========================================
	// 3. 仕上げの魔法（Bloomで光を溢れさせて画面に合成！）
	// ==========================================
	bloom_->Execute();
	bloom_->DrawResult();
}




