
#include "Application/Character/Player/Player.h"
#include "GlobalValiables.h"
#include <GameScene.h>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

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

	model3D_->GetTransform().scale = globalVars->GetVector3Value(groupName, "Scale");
	model3D_->GetTransform().rotate = globalVars->GetVector3Value(groupName, "Rotate");
	model3D_->GetTransform().translate = globalVars->GetVector3Value(groupName, "Translate");
}

/*--------------------
更新処理
------------------------*/
void Player::Update() {

    if (!GameScene::IsPaused()) {
            Input* input = Input::GetInstance();

            if (input->PushKey(DIK_W) || input->PushKey(DIK_UP)) {
                gamePos_.y = 1.f; // 上レーン
            }
            // 離している時は下レーン
            else {
                gamePos_.y = 0.0f; // 下レーン
            }
    }

    GlobalVariables* globalVars = GlobalVariables::GetInstance();
    Vector3 baseScale = globalVars->GetVector3Value("Player", "Scale");
    Vector3 baseRotate = globalVars->GetVector3Value("Player", "Rotate");
    Vector3 baseTranslate = globalVars->GetVector3Value("Player", "Translate");

    // ImGuiからの基本値 + ゲーム内の変動値(gamePos_) を足して最終的な座標にする
    model3D_->GetTransform().scale = baseScale;
    model3D_->GetTransform().rotate = baseRotate;
    model3D_->GetTransform().translate.x = baseTranslate.x + gamePos_.x;
    model3D_->GetTransform().translate.y = baseTranslate.y + gamePos_.y;
    model3D_->GetTransform().translate.z = baseTranslate.z + gamePos_.z;

    model3D_->Update();
}

void Player::DrawImGui() {
#ifdef USE_IMGUI
    // 独立したウィンドウを作成
    ImGui::Begin("Player Settings");

    GlobalVariables* global = GlobalVariables::GetInstance();
    const std::string groupName = "Player";

    Vector3 scale = model3D_->GetTransform().scale;
    Vector3 rotate = model3D_->GetTransform().rotate;
    Vector3 translate = global->GetVector3Value(groupName, "Translate");

    if (ImGui::DragFloat3("Scale", &scale.x, 0.01f)) {
        global->SetValue(groupName, "Scale", scale);
    }
    if (ImGui::DragFloat3("Rotate", &rotate.x, 0.01f)) {
        global->SetValue(groupName, "Rotate", rotate);
    }
    if (ImGui::DragFloat3("Translate", &translate.x, 0.1f)) {
        global->SetValue(groupName, "Translate", translate);
    }

    ImGui::Separator(); // 区切り線

    // セーブボタン
    if (ImGui::Button("SAVE PLAYER SETTINGS (JSON)")) {
        global->SaveFile(groupName);
    }

    // ウィンドウを閉じる
    ImGui::End();
#endif
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