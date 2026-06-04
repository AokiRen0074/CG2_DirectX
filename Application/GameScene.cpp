#include "GameScene.h"
#include "Input/Input.h"
#include "2D/TextureManager.h"
#include <cstdlib>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

GameScene::~GameScene() {
    delete sprite_;
    delete triangle1_;
    delete triangle2_;
    for (int i = 0; i < kNumParticles; i++) {
        delete particles_[i].obj;
    }
    delete debugCamera_;
}

void GameScene::Initialize(DirectXCommon* dxCommon) {
    debugCamera_ = new DebugCamera();
    debugCamera_->Initialize();

    // テクスチャ読み込み
    texture1_ = TextureManager::Load("Resources/uvChecker.png");
    texture2_ = TextureManager::Load("Resources/monsterBall.png");

    // ===================================
    // モード1：評価用の初期化
    // ===================================
    triangle1_ = new Object3d();
    triangle1_->Initialize(dxCommon);
    triangle1_->SetTextureHandle(texture1_);
    triangle1_->GetTransform().translate = { -1.0f, 0.0f, 0.0f };

    triangle2_ = new Object3d();
    triangle2_->Initialize(dxCommon);
    triangle2_->SetTextureHandle(texture2_);
    triangle2_->GetTransform().translate = { 1.0f, 0.0f, 2.0f }; // 少し奥に配置

    // ===================================
    // モード2：映像演出用の初期化
    // ===================================
    auto RandFloat = []() { return (float)rand() / RAND_MAX; };
    for (int i = 0; i < kNumParticles; i++) {
        particles_[i].obj = new Object3d();
        particles_[i].obj->Initialize(dxCommon);
        particles_[i].obj->GetMaterialData()->color = { 0.1f, 0.8f, 0.9f, 1.0f };
        particles_[i].obj->GetMaterialData()->enableLighting = 2;
        // 動画のような青緑色に設定



        particles_[i].obj->GetTransform().translate = { (RandFloat() - 0.5f) * 30.0f, (RandFloat() - 0.5f) * 30.0f, RandFloat() * 50.0f };
        particles_[i].obj->GetTransform().rotate = { RandFloat() * 6.28f, RandFloat() * 6.28f, RandFloat() * 6.28f };
        particles_[i].velocity = { 0.0f, 0.0f, -0.1f - (RandFloat() * 0.2f) };
        particles_[i].rotSpeed = { (RandFloat() - 0.5f) * 0.1f, (RandFloat() - 0.5f) * 0.1f, (RandFloat() - 0.5f) * 0.1f };
    }
}

void GameScene::Update() {
    debugCamera_->Update();

    // ★ Enterキーでモード切り替え！
    if (Input::GetInstance()->TriggerKey(DIK_RETURN)) {
        if (currentMode_ == SceneMode::Evaluation) {
            currentMode_ = SceneMode::Presentation;
        }
        else {
            currentMode_ = SceneMode::Evaluation;
        }
    }

    // ===================================
    // モード1：評価用の更新処理
    // ===================================
    if (currentMode_ == SceneMode::Evaluation) {
#ifdef USE_IMGUI
        ImGui::Begin("Evaluation Tasks");
        ImGui::Text("Press [Enter] to switch to Presentation Mode");
        ImGui::Separator();

        const char* texNames[] = { "Texture 1", "Texture 2" };

        ImGui::Text("Triangle 1 (Front)");
        ImGui::DragFloat3("Pos 1", &triangle1_->GetTransform().translate.x, 0.01f);
        ImGui::DragFloat3("Rot 1", &triangle1_->GetTransform().rotate.x, 0.01f);
        if (ImGui::Combo("Tex 1", &texIndex1_, texNames, 2)) {
            triangle1_->SetTextureHandle(texIndex1_ == 0 ? texture1_ : texture2_);
        }

        ImGui::Separator();
        ImGui::Text("Triangle 2 (Back - Test Depth)");
        ImGui::DragFloat3("Pos 2", &triangle2_->GetTransform().translate.x, 0.01f);
        ImGui::DragFloat3("Rot 2", &triangle2_->GetTransform().rotate.x, 0.01f);
        if (ImGui::Combo("Tex 2", &texIndex2_, texNames, 2)) {
            triangle2_->SetTextureHandle(texIndex2_ == 0 ? texture1_ : texture2_);
        }
        ImGui::End();
#endif
        triangle1_->SetCameraMatrix(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
        triangle1_->Update();

        triangle2_->SetCameraMatrix(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
        triangle2_->Update();
    }
    // ===================================
    // モード2：映像演出用の更新処理
    // ===================================
    else if (currentMode_ == SceneMode::Presentation) {
#ifdef USE_IMGUI
        ImGui::Begin("Presentation Mode");
        ImGui::Text("Press [Enter] to switch to Evaluation Mode");
        ImGui::End();
#endif
        auto RandFloat = []() { return (float)rand() / RAND_MAX; };
        for (int i = 0; i < kNumParticles; i++) {
            auto& transform = particles_[i].obj->GetTransform();
            transform.translate.x += particles_[i].velocity.x;
            transform.translate.y += particles_[i].velocity.y;
            transform.translate.z += particles_[i].velocity.z;
            transform.rotate.x += particles_[i].rotSpeed.x;
            transform.rotate.y += particles_[i].rotSpeed.y;
            transform.rotate.z += particles_[i].rotSpeed.z;

            // 奥にワープさせるループ処理
            if (transform.translate.z < -5.0f) {
                transform.translate.z = 50.0f;
                transform.translate.x = (RandFloat() - 0.5f) * 30.0f;
                transform.translate.y = (RandFloat() - 0.5f) * 30.0f;
            }

            particles_[i].obj->SetCameraMatrix(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
            particles_[i].obj->Update();
        }
    }
}

void GameScene::Draw() {
    if (currentMode_ == SceneMode::Evaluation) {
        triangle1_->Draw();
        triangle2_->Draw();
    }
    else if (currentMode_ == SceneMode::Presentation) {
        for (int i = 0; i < kNumParticles; i++) {
            particles_[i].obj->Draw();
        }
    }
}