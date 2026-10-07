#include "GameScene.h"
#include "Input/Input.h"
#include "2D/TextureManager.h"
#include "AxisIndicator.h"
#include "GlobalValiables.h"
#include "Skydome.h"


#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

GameScene::~GameScene() {
	delete debugCamera_;
	delete bloom_;
	delete object3d_;
	delete rhythmJudge_;
}




void GameScene::Initialize(DirectXCommon* dxCommon) {



	GlobalVariables::GetInstance()->LoadFiles();

	dxCommon_ = dxCommon;

	Object3d::StaticInitialize(dxCommon);

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
	viewProjection_.translation_.z = -300.0f; 
	viewProjection_.translation_.x = 2.0f;  
	viewProjection_.translation_.y = 5.5f; 
	viewProjection_.UpdateMatrix();

	/*-------------------------------
	Bloomの生成と初期化
	----------------------------------*/
	bloom_ = new Bloom();
	bloom_->Initialize(dxCommon, 1280, 720);

	/*------------------
	プレイヤー
	----------------------*/
	player_ = new Player();
	player_->Initialize();

	/*--------------------
	敵
	-----------------------*/
	enemyManager_ = new EnemyManager();
	enemyManager_->Initialize();

	/*--------------------------
	攻撃判定
	-----------------------*/
	rhythmJudge_ = new RhythmJudge();
	rhythmJudge_->Initialize();

	/*-----------------------
	軸表示
	------------------------*/
	AxisIndicator::GetInstance()->Initialize();
	AxisIndicator::GetInstance()->SetVisible(true);
	AxisIndicator::GetInstance()->SetTargetCamera(&viewProjection_);



	/*-------------------------
	音
	-------------------------------*/
	//bgmSound_ = Audio::GetInstance()->SoundLoadWave("Sounds/mainBGM.wav");
	//warpSound_ = Audio::GetInstance()->SoundLoadWave("Sounds/warp.wav");

	//bgmVoice_ = Audio::GetInstance()->SoundPlayWave(bgmSound_, true);

}

bool GameScene::isPaused_ = false;

void GameScene::Update() {



	// ==========================================
//  ImGuiの描画
// ==========================================
#ifdef USE_IMGUI




	ImGui::Begin("Debug Menu");
	ImGui::Checkbox("Pause Game (Stop & Edit)", &isPaused_);

	ImGui::End();

	// グローバル変数の設定
	/*
	if (ImGui::TreeNodeEx("Global Variables")) {
		GlobalVariables::GetInstance()->Update();
		ImGui::TreePop();
	}
	*/

	enemyManager_->DrawImGui();

	player_->DrawImGui();



#endif

	if (debugCamera_) {
		debugCamera_->Update();
	}

	viewProjection_.UpdateMatrix();

	//GlobalVariables::GetInstance()->Update();

	/*----------------------
	プレイヤー
	------------------------*/
	player_->Update();

	/*----------------------
	敵
	-----------------------*/
	enemyManager_->Update();


	// 衝突判定

	if (!isPaused_) {
		rhythmJudge_->Update(player_, enemyManager_);
	}

	// 軸表示
	//AxisIndicator::GetInstance()->Update();
}

/*--------------------------
衝突判定
---------------------------*/


void GameScene::Draw() {
	// ==========================================
	// 普通のやつ
	// ==========================================

	// プレイヤー
	player_->Draw(worldTransform_, viewProjection_, textureHandle_);

	// 敵
	enemyManager_->Draw(worldTransform_, viewProjection_, textureHandle_);

	// ==========================================
	//  ネオン
	// ==========================================
	bloom_->PreDraw();


	// HDRキャンバスへの書き込み終了、普通の画面に戻る
	bloom_->PostDraw();

	bloom_->Execute();
	bloom_->DrawResult();

}