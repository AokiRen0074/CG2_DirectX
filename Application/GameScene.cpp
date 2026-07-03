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

/*----------------------------------------
衝突判定と応答
------------------------------------*/
void GameScene::CheckAllCollision() {

	if (!player_ || !enemy_) return;

	// 衝突判定AとBの座標
	Vector3 posA, posB;

	// 自弾リストの取得
	const std::list<PlayerBullet*>& playerBullets = player_->GetBullets();

	// 敵弾リストの取得
	const std::list<EnemyBullet*>& enemyBullets = enemy_->GetBullets();

	/*---------------------------------
	 自キャラと敵弾の当たり判定
	 ------------------------------*/
	 // 自キャラの座標
	posA = player_->GetworldPosition();



	const float playerRadius = 1.0f;
	const float enemyRadius = 1.0f;
	const float pBulletRadius = 1.0f;
	const float eBulletRadius = 1.0f;


	// 自キャラと敵弾全ての当たり判定
	for (EnemyBullet* bullet : enemyBullets) {

		// すでにない場合は判定しない
		if (bullet->IsDead()) continue;

		// 敵弾の座標
		posB = bullet->GetWorldBulletPosition();

		float dx = posB.x - posA.x;
		float dy = posB.y - posA.y;
		float dz = posB.z - posA.z;
		float distance = std::sqrt(dx * dx + dy * dy + dz * dz);

		//球と球の交差判定
		if (distance <= playerRadius + eBulletRadius) {
			// 自キャラの衝突時コールバックを呼び出す
			player_->OnCollision();
			// 敵弾の衝突時コールバックを呼び出す
			bullet->OnCollision();
		}
	}
	/*-------------------------
	自弾と敵キャラの当たり判定
	---------------------------*/
	Vector3 enemyPos = enemy_->GetWorldPosition();

	for (PlayerBullet* pBullet : playerBullets) {
		if (pBullet->IsDead()) continue; // 死んでいる弾はスルー

		Vector3 pBulletPos = pBullet->GetPlayerBulletWorldPos();

		float dx = pBulletPos.x - enemyPos.x;
		float dy = pBulletPos.y - enemyPos.y;
		float dz = pBulletPos.z - enemyPos.z;
		float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

		// 当たっていたらお互いのコールバックを呼ぶ
		if (dist <= enemyRadius + pBulletRadius) {
			enemy_->OnCollision();
			pBullet->OnCollision();
		}
	}

	/*----------------------------------
	自弾と敵弾の当たり判定
	----------------------------*/
	for (PlayerBullet* pBullet : playerBullets) {
		if (pBullet->IsDead()) continue;
		Vector3 pBulletPos = pBullet->GetPlayerBulletWorldPos();

		for (EnemyBullet* eBullet : enemyBullets) {
			if (eBullet->IsDead()) continue;
			Vector3 eBulletPos = eBullet->GetWorldBulletPosition();

			// 弾同士の距離計算
			float dx = eBulletPos.x - pBulletPos.x;
			float dy = eBulletPos.y - pBulletPos.y;
			float dz = eBulletPos.z - pBulletPos.z;
			float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

			// 当たっていたら両方の弾を消す
			if (dist <= pBulletRadius + eBulletRadius) {
				pBullet->OnCollision();
				eBullet->OnCollision();
			}
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