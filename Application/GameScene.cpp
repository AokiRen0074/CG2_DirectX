#include "GameScene.h"
#include "Input/Input.h"
#include "2D/TextureManager.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif




GameScene::~GameScene() {
    delete sprite_;
    delete object3d_;
    delete triangle1_;
    delete triangle2_;
    delete debugCamera_;
}

void GameScene::Initialize(DirectXCommon* dxCommon) {
    debugCamera_ = new DebugCamera();
    debugCamera_->Initialize();


    texture1_ = TextureManager::Load("Resources/uvChecker.png");
    texture2_ = TextureManager::Load("Resources/monsterBall.png");


    triangle1_ = new Object3d();
    triangle1_->Initialize(dxCommon);
    triangle1_->SetTextureHandle(texture1_);
    triangle1_->GetTransform().translate = { -1.0f, 0.0f, 0.0f };

    
    triangle2_ = new Object3d();
    triangle2_->Initialize(dxCommon);
    triangle2_->SetTextureHandle(texture2_);
    triangle2_->GetTransform().translate = { 1.0f, 0.0f, 2.0f }; // Zをズラして奥に配置
}
void GameScene::Update() {
    debugCamera_->Update();

#ifdef USE_IMGUI
    ImGui::Begin("Evaluation Tasks");

    const char* texNames[] = { "Texture 1", "Texture 2" };

    ImGui::Text("Triangle 1 (Front)");
    // ★ 三角形を動かす (4点)
    ImGui::DragFloat3("Pos 1", &triangle1_->GetTransform().translate.x, 0.01f);
    ImGui::DragFloat3("Rot 1", &triangle1_->GetTransform().rotate.x, 0.01f);
    // ★ Textureの動的切り替え (5点)
    if (ImGui::Combo("Tex 1", &texIndex1_, texNames, 2)) {
        triangle1_->SetTextureHandle(texIndex1_ == 0 ? texture1_ : texture2_);
    }

    ImGui::Separator();

    ImGui::Text("Triangle 2 (Back - Test Depth)");
    // 奥のオブジェクトを手前に移動させて、DepthBufferが効いているか確認できる
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

void GameScene::Draw() {
    triangle1_->Draw();
    triangle2_->Draw();
}