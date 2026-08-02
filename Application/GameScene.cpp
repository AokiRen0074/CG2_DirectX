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

    delete teapot_;
    delete bunny_;
    delete suzanne_;
    delete sphere_;
    delete multiMesh_;
    delete multiMaterial_;
    delete gamePad_;
    delete pathTracer_;
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

    teapot_ = new Object3d();
    teapot_->Initialize("Resources", "teapot.obj");

    bunny_ = new Object3d();
    bunny_->Initialize("Resources", "bunny.obj");


    suzanne_ = new Object3d();
    suzanne_->Initialize("Resources", "suzanne.obj");

    Sphere::StaticInitialize(dxCommon); 
    sphere_ = new Sphere();
    sphere_->Initialize();             
    sphere_->GetTransform().translate.x =8.0f;

    multiMesh_ = new Object3d();
    multiMesh_->Initialize("Resources", "multiMesh.obj");


    multiMaterial_ = new Object3d();
    multiMaterial_->Initialize("Resources", "multiMaterial.obj");


  //  modelData_ = new ModelData();

  //  *modelData_ = LoadObjectFile("Resources", "Player.obj");
    textureHandle_ = TextureManager::Load("Resources/uvChecker.png");
    teapotTexture_ = TextureManager::Load("Resources/checkerBoard.png");
    ballTexture_ = TextureManager::Load("Resources/monsterBall.png");

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
   パストレーサー
   ------------------------------*/
   PathTracer::StaticInitialize(dxCommon);
   pathTracer_ = new PathTracer();
   pathTracer_->Initialize();

   /*------------------
   ゲームパッド
   --------------------*/
   gamePad_ = new GamePad();

    // 音の読み込み
   soundData_ = Audio::GetInstance()->SoundLoadWave("Resources/Alarm01.wav");
}

void GameScene::Update() {

    if (gamePad_) {
        gamePad_->Update();
    }

#ifdef USE_IMGUI
    ImGui::ShowDemoWindow();
    ImGui::Begin("Scene Selector");
    ImGui::Combo("Select Model", &currentModelType_, modelNames_, 7);
    ImGui::End();

    ImGui::Begin("Sound");
    ImGui::Text("Press [SPACE] key to Play Sound");
    ImGui::Separator();

    // 鳴っているときだけ赤文字を出す
    if (isSoundPlaying_) {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), ">>> SOUND PLAYING <<<");
    }
    ImGui::End();

    if (gamePad_) {
        gamePad_->DrawImGui();
    }

#endif

    if (currentModelType_ == 0) {
        object3d_->Update();
        sprite_->Update();
        sphere_->Update();
    }
    else if (currentModelType_ == 1) {
        teapot_->Update();
    }
    else if (currentModelType_ == 2) {
        bunny_->Update();
    }
    else if (currentModelType_ == 3) {
        suzanne_->Update();
    }
    else if (currentModelType_ == 4){
        multiMesh_->Update();
    }
    else if (currentModelType_ == 5) {
        multiMaterial_ -> Update();
    }
    else if (currentModelType_ == 6) {
        pathTracer_->Update();
    }

    // カメラの更新
    debugCamera_->Update();

    // 音
    if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
        Audio::GetInstance()->SoundPlayWave(soundData_);

        isSoundPlaying_ = true;
        soundVisualTimer_ = 240;
    }

    // タイマーのカウントダウン
    if (soundVisualTimer_ > 0) {
        soundVisualTimer_--;
    }
    else {
        isSoundPlaying_ = false;
    }

    /*------------------
    自キャラ更新
    ----------------------*/
   // player_->Update();


    // オブジェクトの更新
  object3d_->SetCameraMatrix(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());

  object3d_->SetCameraMatrix(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
  teapot_->SetCameraMatrix(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
  bunny_->SetCameraMatrix(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
  suzanne_->SetCameraMatrix(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
  sphere_->SetCameraMatrix(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
  multiMesh_->SetCameraMatrix(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
  multiMaterial_->SetCameraMatrix(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
 

  //sprite_->Update();

#ifdef USE_IMGUI
    ImGui::ShowDemoWindow();
#endif
}

void GameScene::Draw() {

    /*-------------------
    自キャラ描画
    --------------------*/
   // player_->Draw(viewProjection_);

    // 3Dモデル描画
    WorldTransform dummyTransform;
    dummyTransform.Initialize();


    if (currentModelType_ == 0) {
        object3d_->Draw(dummyTransform, viewProjection_, textureHandle_);
        sprite_->Draw();
        sphere_->Draw(dummyTransform, viewProjection_, textureHandle_);
    }
    else if (currentModelType_ == 1) {
        teapot_->Draw(dummyTransform, viewProjection_, teapotTexture_);
    }
    else if (currentModelType_ == 2) {
        bunny_->Draw(dummyTransform, viewProjection_, textureHandle_);
    }
    else if (currentModelType_ == 3) {
        suzanne_->Draw(dummyTransform, viewProjection_);
    }
    else if (currentModelType_ == 4) {
        multiMesh_->Draw(dummyTransform, viewProjection_, textureHandle_);
    }
    else if (currentModelType_ == 5) {
        multiMaterial_->Draw(dummyTransform, viewProjection_, ballTexture_);
    }
    else if (currentModelType_ == 6) {
        pathTracer_->Draw();
    }
    

    // 2Dスプライト描画
   //  sprite_->Draw();

   
}