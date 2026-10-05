#include "Enemy.h"
#include "GlobalValiables.h"
#include <GameScene.h>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void Enemy::Initialize(const Vector3& position) {
    GlobalVariables* globalVars = GlobalVariables::GetInstance();

    model3D_ = new Object3d();
    model3D_->Initialize("Resources", "fence.obj");

    const std::string groupName = "Enemy";
    globalVars->CreateGroup(groupName);

    globalVars->AddItem(groupName, "Scale", Vector3{ 1.0f, 1.0f, 1.0f });
    globalVars->AddItem(groupName, "Rotate", Vector3{ 0.0f, 1.57f, 0.0f });
    globalVars->AddItem(groupName, "Translate", position);

    ApplyGlobalVariables();
}

void Enemy::ApplyGlobalVariables() {
    GlobalVariables* globalVars = GlobalVariables::GetInstance();
    const std::string groupName = "Enemy";

    model3D_->GetTransform().scale = globalVars->GetVector3Value(groupName, "Scale");
    model3D_->GetTransform().rotate = globalVars->GetVector3Value(groupName, "Rotate");
}

void Enemy::Update() {
    ApplyGlobalVariables();

    if (!GameScene::IsPaused()) {
        gamePos_.x -= speed_;
    }

    GlobalVariables* globalVars = GlobalVariables::GetInstance();
    Vector3 baseTranslate = globalVars->GetVector3Value("Enemy", "Translate");

    model3D_->GetTransform().translate.x = baseTranslate.x + gamePos_.x;
    model3D_->GetTransform().translate.y = baseTranslate.y + gamePos_.y;
    model3D_->GetTransform().translate.z = baseTranslate.z + gamePos_.z;

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