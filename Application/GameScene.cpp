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
	viewProjection_.translation_.z = -20.0f;
	viewProjection_.UpdateMatrix();

	/*-------------------------------
	Bloomの生成と初期化
	----------------------------------*/
	bloom_ = new Bloom();
	bloom_->Initialize(dxCommon,1280,720);

	/*----------------------
	3Dオブジェクトの生成、読み込み
	------------------------------*/
	textureHandle_ = TextureManager::Load("Resources/uvChecker.png");

	// 3Dオブジェクトの生成
	object3d_ = new Object3d();
	object3d_->Initialize("Resources", "fence.obj");
	object3d_->GetTransform().rotate.y = 3.14f;
	object3d_->GetTransform().scale = { 0.5f,0.5f,0.5f };

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();

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

void GameScene::Update() {



	// ==========================================
//  ImGuiの描画
// ==========================================
#ifdef USE_IMGUI

	ImGui::Begin("Master Control", nullptr, ImGuiWindowFlags_MenuBar);


	

	// グローバル変数の設定
	/*
	if (ImGui::TreeNodeEx("Global Variables")) {
		GlobalVariables::GetInstance()->Update();
		ImGui::TreePop();
	}
	*/

	

	ImGui::End(); // Master Controlの終了

#endif

	if (debugCamera_) {
		debugCamera_->Update();
	}

	viewProjection_.UpdateMatrix();

	/*-----------------------------
	3Dオブジェクトの更新
	----------------------------*/
	if (object3d_) {
		//object3d_->SetCameraMatrix(viewProjection_.matView, viewProjection_.matProjection);

		object3d_->Update();
	}

	// 軸表示
	//AxisIndicator::GetInstance()->Update();
}

void GameScene::Draw() {
	// ==========================================
	// 普通のやつ
	// ==========================================

	/*-------------------------
	3Dオブジェクトの更新
	----------------------------*/
	if (object3d_) {
		object3d_->Draw(worldTransform_, viewProjection_, textureHandle_);
	}


	// ==========================================
	//  ネオン
	// ==========================================
	bloom_->PreDraw();


	// HDRキャンバスへの書き込み終了、普通の画面に戻る
	bloom_->PostDraw();

	bloom_->Execute();
	bloom_->DrawResult();

}