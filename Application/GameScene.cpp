#include "GameScene.h"
#include "Input/Input.h"
#include "2D/TextureManager.h"
#include "Application/Character/Player.h"
#include "AxisIndicator.h"
#include "GlobalValiables.h"
#include <cmath>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif







GameScene::~GameScene() {
	delete debugCamera_;
	delete player_;
	delete enemy_;
	//delete bulletModel_;
}

void GameScene::Initialize(DirectXCommon* dxCommon) {

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
	弾
	------------------------------*/



	/*-----------------------
	軸表示
	------------------------*/
	AxisIndicator::GetInstance()->Initialize();

	// 軸方向の表示を有効にする
	AxisIndicator::GetInstance()->SetVisible(true);

	// 軸方向表示が参照するビュープロジェクションの指定
	AxisIndicator::GetInstance()->SetTargetCamera(&viewProjection_);

}


void GameScene::CheckCollisionPair(Collider* colliderA, Collider* colliderB) {
	Vector3 posA = colliderA->GetWorldPosition();
	Vector3 posB = colliderB->GetWorldPosition();

	float dx = posB.x - posA.x;
	float dy = posB.y - posA.y;
	float dz = posB.z - posA.z;
	float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

	if (dist <= colliderA->GetRadius() + colliderB->GetRadius()) {
		// コライダーAとBの衝突時コールバックを呼び出す
		colliderA->OnCollision();
		colliderB->OnCollision();
	}
}

void GameScene::CheckAllCollision() {
	if (!player_ || !enemy_) return;

	const std::list<PlayerBullet*>& playerBullets = player_->GetBullets();
	const std::list<EnemyBullet*>& enemyBullets = enemy_->GetBullets();

	// ===============================================
	// 自キャラと敵弾の当たり判定
	// ===============================================
	for (EnemyBullet* bullet : enemyBullets) {
		if (bullet->IsDead()) continue;
		CheckCollisionPair(player_, bullet);
	}

	// ===============================================
	// 自弾と敵キャラの当たり判定
	// ===============================================
	for (PlayerBullet* pBullet : playerBullets) {
		if (pBullet->IsDead()) continue;
		CheckCollisionPair(pBullet, enemy_);
	}

	// ===============================================
	// 自弾と敵弾の当たり判定
	// ===============================================
	for (PlayerBullet* pBullet : playerBullets) {
		if (pBullet->IsDead()) continue;

		for (EnemyBullet* eBullet : enemyBullets) {
			if (eBullet->IsDead()) continue;
			CheckCollisionPair(pBullet, eBullet);
		}
	}
}
void GameScene::Update() {
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


	// オブジェクトの更新
	if (isDebugCameraActive_ && debugCamera_ != nullptr) {
		debugCamera_->Update();

		// ここで安全に取得する
		object3d_->SetCameraMatrix(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
	}

	AxisIndicator::GetInstance()->Update();

	CheckAllCollision();


#ifdef USE_IMGUI
	ImGui::ShowDemoWindow();
	GlobalVariables::GetInstance()->Update();
#endif
}

void GameScene::Draw() {

	// 軸方向描画
	AxisIndicator::GetInstance()->Draw();

	/*-------------------
	自キャラ描画
	--------------------*/

	if (enemy_) {
		enemy_->Draw(viewProjection_);
	}

	player_->Draw(viewProjection_);



}