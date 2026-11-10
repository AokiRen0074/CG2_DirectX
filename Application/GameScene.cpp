#include "GameScene.h"
#include "2D/TextureManager.h"
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void GameScene::Initialize(DirectXCommon* dxCommon) {
    WorldTransform::SetDevice(dxCommon->GetDevice());

    // デバッグカメラの行列を、実際のモデル描画にも使う。
    debugCamera_ = std::make_unique<DebugCamera>();
    debugCamera_->Initialize();
    viewProjection_.Initialize();
    viewProjection_.matView = debugCamera_->GetViewMatrix();
    viewProjection_.matProjection = debugCamera_->GetProjectionMatrix();

    // シーンがモデルを所有し、Playerには参照を渡す。
    object3d_ = std::make_unique<Object3d>();
    object3d_->Initialize("Resources", "Player.obj");
    const auto texture = TextureManager::Load("Resources/uvChecker.png");
    player_ = std::make_unique<Player>();
    player_->Initialize(object3d_.get(), texture);
}

void GameScene::Update() {
    debugCamera_->Update();
    viewProjection_.matView = debugCamera_->GetViewMatrix();
    viewProjection_.matProjection = debugCamera_->GetProjectionMatrix();

    player_->Update();
#ifdef USE_IMGUI
    ImGui::ShowDemoWindow();
#endif
}

void GameScene::Draw() {
    player_->Draw(viewProjection_);
}
