#include "GameScene.h"
#include "Input/Input.h"
#include "2D/TextureManager.h"
#include "Player.h"

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


    object3d_ = new Object3d();
    Object3d::StaticInitialize(dxCommon);

    object3d_->GetTransform().translate.x = 16.0f;
    object3d_->GetTransform().translate.y = 2.8f;
    object3d_->GetTransform().rotate.y = 3.14f;
    object3d_->GetTransform().scale = { 2.0f,2.0f,1.0f };

    object3d_->Initialize("Resources","plane.obj");


  //  modelData_ = new ModelData();

  //  *modelData_ = LoadObjectFile("Resources", "Player.obj");
    textureHandle_ = TextureManager::Load("Resources/uvChecker.png");

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
   player_->Initialize(object3d_, textureHandle_);

   /*--------------------------
   デバッグカメラ
   ------------------------------*/

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
  object3d_->SetCameraMatrix(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
  object3d_->Update();
   sprite_->Update();

#ifdef USE_IMGUI
    ImGui::ShowDemoWindow();
#endif
}

void GameScene::Draw() {

    /*-------------------
    自キャラ描画
    --------------------*/
    player_->Draw(viewProjection_);

    // 3Dモデル描画
    WorldTransform dummyTransform;
    dummyTransform.Initialize();

    object3d_->Draw(dummyTransform, viewProjection_, textureHandle_);

    // 2Dスプライト描画
     sprite_->Draw();
}