
#include "Application/Character/Player/Player.h"
#include "GlobalValiables.h"
#include <GameScene.h>

/*-------------------------
初期化
-----------------------*/
void Player::Initialize() {
	GlobalVariables* globalVars = GlobalVariables::GetInstance();

	model3D_ = new Object3d();
	model3D_->Initialize("Resources", "cube.obj");

	// グローバル変数のグループ作成と初期値の追加
	const std::string groupName = "Player";
	globalVars->CreateGroup(groupName);

	
	globalVars->AddItem(groupName, "Scale", Vector3{ 1.0f, 1.0f, 1.0f });
	globalVars->AddItem(groupName, "Rotate", Vector3{ 0.0f, 1.57f, 0.0f });
	globalVars->AddItem(groupName, "Translate", Vector3{ -2.0f, 0.0f, 0.0f });

	// 初回の値適用
	ApplyGlobalVariables();
}

void Player::ApplyGlobalVariables() {
	GlobalVariables* globalVars = GlobalVariables::GetInstance(); 
	const std::string groupName = "Player";

	model3D_->GetTransform().scale = globalVars->GetVector3Value(groupName, "Scale"); //[cite: 11]
	model3D_->GetTransform().rotate = globalVars->GetVector3Value(groupName, "Rotate"); //[cite: 11]
	model3D_->GetTransform().translate = globalVars->GetVector3Value(groupName, "Translate"); //[cite: 11]
}

/*--------------------
更新処理
------------------------*/
void Player::Update() {

    if (!GameScene::IsPaused()) {
        const float kGravity = -0.05f;
        const float kJumpVelocity = 0.8f;
        if (Input::GetInstance()->TriggerKey(DIK_SPACE) && !isJumping_) {
            velocityY_ = kJumpVelocity;
            isJumping_ = true;
        }
        velocityY_ += kGravity;
        gamePos_.y += velocityY_;

        if (gamePos_.y <= 0.0f) {
            gamePos_.y = 0.0f;
            velocityY_ = 0.0f;
            isJumping_ = false;
        }
    }

    GlobalVariables* globalVars = GlobalVariables::GetInstance(); 
        Vector3 baseScale = globalVars->GetVector3Value("Player", "Scale");
        Vector3 baseRotate = globalVars->GetVector3Value("Player", "Rotate"); 
        Vector3 baseTranslate = globalVars->GetVector3Value("Player", "Translate"); 

    model3D_->GetTransform().scale = baseScale;
    model3D_->GetTransform().rotate = baseRotate;
    model3D_->GetTransform().translate.x = baseTranslate.x + gamePos_.x;
    model3D_->GetTransform().translate.y = baseTranslate.y + gamePos_.y;
    model3D_->GetTransform().translate.z = baseTranslate.z + gamePos_.z;

    model3D_->Update();
}


/*-----------------------
描画処理
-------------------------*/
void Player::Draw(const WorldTransform& worldTransform, const ViewProjection& viewProjection, uint32_t textureHandle) {
	model3D_->Draw(worldTransform, viewProjection, textureHandle);
}

void Player::OnCollision() {
	// 敵とぶつかった時の処理
}