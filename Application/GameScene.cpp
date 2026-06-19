#include "GameScene.h"
#include "Input/Input.h"
#include "2D/TextureManager.h"
#include "Application/Character/Player.h"
#include "AxisIndicator.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif



GameScene::~GameScene() {
  //  delete sprite_;
   // delete object3d_;
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
    object3d_->Initialize("Resources","Player.obj");


  //  modelData_ = new ModelData();

  //  *modelData_ = LoadObjectFile("Resources", "Player.obj");
    textureHandle_ = TextureManager::Load("Resources/uvChecker.png");

    /*----------------------
    スプライトの生成と初期化
    -------------------------*/
  // uint32_t textureHandle = TextureManager::Load("Resources/uvChecker.png"); 

  // sprite_ = Sprite::Create(textureHandle, { 100.0f, 50.0f });

   /*-------------------------------
   自キャラ生成と初期化
   ----------------------------------*/

   // 自キャラの生成
   player_ = new Player();

   // 自キャラの初期化
   player_->Initialize(object3d_, textureHandle_);



   /*-----------------------
   軸表示
   ------------------------*/
   AxisIndicator::GetInstance()->Initialize();

   // 軸方向の表示を有効にする
   AxisIndicator::GetInstance()->SetVisible(true);

   // 軸方向表示が参照するビュープロジェクションの指定
   AxisIndicator::GetInstance()->SetTargetCamera(&viewProjection_);


    // 音の読み込み
  //  soundData_ = Audio::GetInstance()->SoundLoadWave("Resources/Alarm01.wav");
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



    if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
        // Audio::GetInstance()->SoundPlayWave(soundData_);
    }

    /*------------------
    自キャラ更新
    ----------------------*/
    player_->Update();


    // オブジェクトの更新
    if (isDebugCameraActive_ && debugCamera_ != nullptr) {
        debugCamera_->Update();

        // ここで安全に取得する
        object3d_->SetCameraMatrix(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
    }

    AxisIndicator::GetInstance()->Update();

  //object3d_->Update();
   //prite_->Update();

#ifdef USE_IMGUI
    ImGui::ShowDemoWindow();
#endif
}

void GameScene::Draw() {

    // 軸方向描画
    AxisIndicator::GetInstance()->Draw();

    /*-------------------
    自キャラ描画
    --------------------*/
    player_->Draw(viewProjection_);

    // 3Dモデル描画
  //object3d_->Draw();

    // 2Dスプライト描画
     //sprite_->Draw();
}