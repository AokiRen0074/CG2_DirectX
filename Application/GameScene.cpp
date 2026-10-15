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
    delete effectBloom_;
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
	// 横と縦に動ける固定カメラ。奥行きは3Dモデルで表現する。
	viewProjection_.translation_ = { 0.0f, 1.0f, -32.0f };
	viewProjection_.UpdateMatrix();

	/*-------------------------------
	Bloomの生成と初期化
	----------------------------------*/
	bloom_ = new Bloom();
	bloom_->Initialize(dxCommon,1280,720);

    // ステージの色は保ち、エフェクトだけをHDRで発光させて重ねる。
    effectRenderer_.Initialize(dxCommon);
    effectBloom_ = new Bloom();
    effectBloom_->Initialize(dxCommon, 1280, 720);
    effectBloom_->EnableEmissionLayer();

	/*----------------------
	3Dオブジェクトの生成、読み込み
	------------------------------*/
	textureHandle_ = TextureManager::Load("Resources/uvChecker.png");

	// 3Dオブジェクトの生成
	object3d_ = new Object3d();
	object3d_->Initialize("Resources", "fence.obj");
	object3d_->GetTransform().rotate.y = 3.14f;
	object3d_->GetTransform().scale = { 0.5f,0.5f,0.5f };

	// 元の確認用フェンスはステージの外に置く。
	object3d_->SetPosition({ 30.0f, 0.0f, 0.0f });
	spearPrototype_.Initialize();

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

	spearPrototype_.Update();



	// ==========================================
//  ImGuiの描画
// ==========================================
#ifdef USE_IMGUI

	// ステージの情報と調整は槍のHUD・F2エディターで表示する。


	

	// グローバル変数の設定
	/*
	if (ImGui::TreeNodeEx("Global Variables")) {
		GlobalVariables::GetInstance()->Update();
		ImGui::TreePop();
	}
	*/

	

	// Master Controlの終了（表示を槍エディターへ統合）

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
		object3d_->SetCameraMatrix(viewProjection_.matView, viewProjection_.matProjection);
	}

	// 軸表示
	//AxisIndicator::GetInstance()->Update();
}

void GameScene::Draw() {

	spearPrototype_.Draw(viewProjection_);

	// ==========================================
	// 普通のやつ
	// ==========================================

	/*-------------------------
	3Dオブジェクトの描画
	----------------------------*/
	if (object3d_) {
		object3d_->Draw(viewProjection_, textureHandle_);
	}


	// ==========================================
	//  ネオン
	// ==========================================
	// 描画済みの画面を取り込み、ボス撃破時だけブラーを加える。
	bloom_->CaptureScene();


	// HDRキャンバスへの書き込み終了、普通の画面に戻る
	const auto center = spearPrototype_.VictoryBlurCenter();
	bloom_->SetRadialBlur(spearPrototype_.VictoryBlurStrength(), center[0], center[1]);

	bloom_->Execute();
	bloom_->DrawResult();

	// HDRの光を深度を保って描く。形がある時だけぼかしと合成を実行する。
    effectBloom_->PreDraw();
    const bool hasEffects = spearPrototype_.DrawEffects(effectRenderer_);
    effectBloom_->PostDraw();
    if (hasEffects) {
        effectBloom_->SetRadialBlur(spearPrototype_.VictoryBlurStrength(), center[0], center[1]);
        effectBloom_->Execute();
        effectBloom_->DrawResult();
    } else {
        dxCommon_->SetBackBufferRenderTarget();
    }

    // 回数表示はポストエフェクトの後に描画して読みやすさを保つ。
	spearPrototype_.DrawDashGauge();

}