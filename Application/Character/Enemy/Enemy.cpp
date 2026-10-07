#include "Enemy.h"
#include "GlobalValiables.h"
#include <GameScene.h>

void Enemy::Initialize(const Vector3& position) {
    model3D_ = new Object3d();
    model3D_->Initialize("Resources", "fence.obj");

    model3D_->GetTransform().translate = position;

    // ScaleとRotateだけImGuiで管理する
    GlobalVariables* globalVars = GlobalVariables::GetInstance();
    const std::string groupName = "Enemy";
    globalVars->CreateGroup(groupName);
    globalVars->AddItem(groupName, "Scale", Vector3{ 1.0f, 1.0f, 1.0f });
    globalVars->AddItem(groupName, "Rotate", Vector3{ 0.0f, 1.57f, 0.0f });
}

void Enemy::ApplyGlobalVariables() {
    GlobalVariables* globalVars = GlobalVariables::GetInstance();
    const std::string groupName = "Enemy";
    // Translateの上書きは絶対にしない！
    model3D_->GetTransform().scale = globalVars->GetVector3Value(groupName, "Scale");
    model3D_->GetTransform().rotate = globalVars->GetVector3Value(groupName, "Rotate");
}

void Enemy::Update() {
    ApplyGlobalVariables();

    if (!GameScene::IsPaused()) {
        // 自分の座標をシンプルに左へ動かすだけ
        model3D_->GetTransform().translate.x -= speed_;
    }

    model3D_->Update();
}



void Enemy::Draw(const WorldTransform& worldTransform, const ViewProjection& viewProjection, uint32_t textureHandle) {
    if (!isDead_) {
        model3D_->Draw(worldTransform, viewProjection, textureHandle);
    }
}

void Enemy::OnCollision() {
    isDead_ = true;
}