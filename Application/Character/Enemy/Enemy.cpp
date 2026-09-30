#include "Enemy.h"
#include "GlobalValiables.h"
#include <GameScene.h>
/*---------------------
更新処理
----------------------*/
void Enemy::Initialize(const Vector3& position) {

    GlobalVariables* globalVars = GlobalVariables::GetInstance();

    // 3Dモデルの生成と初期化
    model3D_ = new Object3d();
    model3D_->Initialize("Resources", "fence.obj");

    // グローバル変数のグループ作成と初期値の追加
    const std::string groupName = "Enemy";
    globalVars->CreateGroup(groupName);


    globalVars->AddItem(groupName, "Scale", Vector3{ 1.0f, 1.0f, 1.0f });
    globalVars->AddItem(groupName, "Rotate", Vector3{ 0.0f, 1.57f, 0.0f });
    globalVars->AddItem(groupName, "Translate", position);

    // 初回の値適用
    ApplyGlobalVariables();
}

void Enemy::ApplyGlobalVariables() {
    GlobalVariables* globalVars = GlobalVariables::GetInstance(); 
    const std::string groupName = "Enemy"; 

    model3D_->GetTransform().scale = globalVars->GetVector3Value(groupName, "Scale"); //[cite: 11]
    model3D_->GetTransform().rotate = globalVars->GetVector3Value(groupName, "Rotate"); //[cite: 11]


}
/*----------------
更新処理
-----------------------*/
void Enemy::Update() {
    ApplyGlobalVariables();

    if (!GameScene::IsPaused()) {
        // 左へ移動
        model3D_->GetTransform().translate.x -= speed_;
        model3D_->Update();
    }

    GlobalVariables* globalVars = GlobalVariables::GetInstance();
    Vector3 baseScale = globalVars->GetVector3Value("Enemy", "Scale");
    Vector3 baseRotate = globalVars->GetVector3Value("Enemy", "Rotate");
    Vector3 baseTranslate = globalVars->GetVector3Value("Enemy", "Translate");

    model3D_->GetTransform().scale = baseScale;
    model3D_->GetTransform().rotate = baseRotate;
    model3D_->GetTransform().translate.x = baseTranslate.x + gamePos_.x;
    model3D_->GetTransform().translate.y = baseTranslate.y + gamePos_.y;
    model3D_->GetTransform().translate.z = baseTranslate.z + gamePos_.z;

    model3D_->Update();
}

/*-------------------
描画処理
--------------------*/
void Enemy::Draw(const WorldTransform& worldTransform, const ViewProjection& viewProjection, uint32_t textureHandle) {
    if (!isDead_) {
        model3D_->Draw(worldTransform, viewProjection, textureHandle);
    }
}

// 衝突判定
void Enemy::OnCollision() {
    isDead_ = true;
}