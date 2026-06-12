#include "GameScene.h"
#include "Input/Input.h"
#include "2D/TextureManager.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif




GameScene::~GameScene() {
    delete sprite_;
    delete object3d_;
    delete debugCamera_;
    delete player_;
}

void GameScene::Initialize(DirectXCommon* dxCommon) {

    



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
    object3d_ = new Object3d();
    Object3d::StaticInitialize(dxCommon);

    object3d_ = Object3d::Create("Resources", "axis.obj");

    /*----------------------
    スプライトの生成と初期化
    -------------------------*/
   uint32_t textureHandle = TextureManager::Load("Resources/uvChecker.png"); 

   sprite_ = Sprite::Create(textureHandle, { 100.0f, 50.0f });

   /*-------------------------------
   自キャラ生成と初期化
   ----------------------------------*/
   // 自キャラの生成
   player_ = new Player();

   // 自キャラの初期化
   player_->Initialize();


    // 音の読み込み
  //  soundData_ = Audio::GetInstance()->SoundLoadWave("Resources/Alarm01.wav");
}

void GameScene::Update() {
    // カメラの更新
    debugCamera_->Update();

    if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
        // Audio::GetInstance()->SoundPlayWave(soundData_);
    }

    /*------------------
    自キャラ更新
    ----------------------*/
    player_->Update();


    // オブジェクトの更新
 // object3d_->SetCameraMatrix(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
  //object3d_->Update();
   //prite_->Update();

#ifdef USE_IMGUI
    ImGui::ShowDemoWindow();
#endif
}

void GameScene::Draw() {

    /*-------------------
    自キャラ描画
    --------------------*/
    player_->Draw();

    // 3Dモデル描画
  //object3d_->Draw();

    // 2Dスプライト描画
     //rite_->Draw();
}