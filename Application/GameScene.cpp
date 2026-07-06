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
	neonText_ = new NeonText();


	neonText_->Initialize(dxCommon_);
	neonText_->Print("NEON", -3.0f, 0.0f, 0.7f);

	bloom_ = new Bloom();
	// 画面サイズ（1280x720）を渡す
	bloom_->Initialize(dxCommon_, WindowApp::kClientWidth, WindowApp::kClientHeight);


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
	object3d_ = new Object3d();
	Object3d::StaticInitialize(dxCommon);
	object3d_->Initialize("Resources", "player.obj");

	textureHandle_ = TextureManager::Load("Resources/uvChecker.png");

	// エネミー
	enemyObject_ = new Object3d();
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
	player_->Initialize(object3d_, textureHandle_);


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
	ImGui::Begin("Neon Control Panel");
	ImGui::SliderFloat("Radius (太さ)", &neonRadius, 0.001f, 0.1f);
	ImGui::SliderFloat("Length Offset (長さ微調整)", &neonLengthOffset, -1.0f, 1.0f);
	ImGui::SliderFloat("Softness (ぼかし)", &neonSoftness, 0.1f, 50.0f);
	ImGui::SliderFloat("Intensity (光の強さ)", &neonIntensity, 0.1f, 20.0f);
	ImGui::ColorEdit3("Color (色)", neonColor);
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
		object3d_->SetCameraMatrix(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
	}


	neonText_->SetMaterial(neonRadius, neonSoftness, neonIntensity, neonColor[0], neonColor[1], neonColor[2], neonLengthOffset);

	if (neonText_ != nullptr) {
		neonText_->SetMaterial(neonRadius, neonSoftness, neonIntensity, neonColor[0], neonColor[1], neonColor[2], neonLengthOffset);

		if (isDebugCameraActive_ && debugCamera_ != nullptr) {
			neonText_->Update(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
		}
		else {
			neonText_->Update(viewProjection_.matView, viewProjection_.matProjection);
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

	// 軸方向描画
	AxisIndicator::GetInstance()->Draw();
	if (enemy_) {
		enemy_->Draw(viewProjection_);
	}
	player_->Draw(viewProjection_); // 普通の3Dプレイヤー


	// ==========================================
	// 2. ネオンの描画（ここからHDRキャンバス R16G16B16A16 に切り替え！）
	// ==========================================
	bloom_->PreDraw(); // キャンバスを切り替え

	if (neonText_ != nullptr) {
		neonText_->Draw();
	}
	// もしネオン自機（neonPlayer_）などがいるなら、それもここでDrawする

	bloom_->PostDraw(); // HDRキャンバスへの書き込み終了


	// ==========================================
	// 3. 仕上げの魔法（ぼかして光を溢れさせ、モニターに合成する）
	// ==========================================
	bloom_->Execute();    // コンピュートシェーダーでぼかし計算
	bloom_->DrawResult(); // モニターに最終結果をドン！と描画
}




