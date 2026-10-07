#include "EnemyManager.h"
#include "GameScene.h" 
#include "GlobalValiables.h"
#include <cstdlib>
#include <ctime>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void EnemyManager::Initialize() {
    spawnTimer_ = 0;
    spawnInterval_ = 60;
    enemySpeed_ = 0.05f;

    // ランダムの初期化
    srand(static_cast<unsigned int>(time(nullptr)));

    GlobalVariables* globalVars = GlobalVariables::GetInstance();
    const std::string groupName = "Enemy";
    globalVars->CreateGroup(groupName);

    // 初期のパラメータをセット
    globalVars->AddItem(groupName, "Scale", Vector3{ 1.0f, 1.0f, 1.0f });
    globalVars->AddItem(groupName, "Rotate", Vector3{ 0.0f, 1.57f, 0.0f });
    globalVars->AddItem(groupName, "Translate", Vector3{ 15.0f, 0.0f, 0.0f });
    // ==========================================

    // 最初の1体をスポーン
    Enemy* newEnemy = new Enemy();
    newEnemy->Initialize({ 15.0f, 0.0f, 0.0f });
    newEnemy->SetSpeed(enemySpeed_);
    enemies_.push_back(newEnemy);
}

void EnemyManager::Update() {
    if (!GameScene::IsPaused()) {
        spawnTimer_++;
        if (spawnTimer_ >= spawnInterval_) {
            Enemy* newEnemy = new Enemy();

   
            int lane = rand() % 2;
            float laneY = (lane == 0) ? 0.0f : 2.5f;

            // 決まった高さを Initialize に渡す
            newEnemy->Initialize({ 15.0f, laneY, 0.0f });
            newEnemy->SetSpeed(enemySpeed_);
            enemies_.push_back(newEnemy);

            spawnTimer_ = 0;
        }
    }

    // 全ての敵を更新
    for (Enemy* enemy : enemies_) {
        enemy->Update();
    }

    // 左端に出た敵を削除
    enemies_.remove_if([](Enemy* enemy) {
        if (enemy->IsDead() || enemy->GetPosition().x < -10.0f) {
            delete enemy;
            return true;
        }
        return false;
        });
}

void EnemyManager::DrawImGui() {
#ifdef USE_IMGUI
    ImGui::Begin("Enemy Manager (MuseDash Control)");

    ImGui::Text("Active Enemies: %d", (int)enemies_.size());
    ImGui::Separator();

    // 湧く間隔とスピードの調整
    ImGui::SliderInt("Spawn Interval (Frames)", &spawnInterval_, 10, 120);
    if (ImGui::SliderFloat("Scroll Speed", &enemySpeed_, 0.01f, 0.3f)) {
        for (Enemy* enemy : enemies_) {
            enemy->SetSpeed(enemySpeed_);
        }
    }

    ImGui::Separator();
    ImGui::Text("Enemy Visual Settings (SRT)");

    GlobalVariables* global = GlobalVariables::GetInstance();
    const std::string groupName = "Enemy";

    // GlobalVariablesから現在の値を取得
    Vector3 scale = global->GetVector3Value(groupName, "Scale");
    Vector3 rotate = global->GetVector3Value(groupName, "Rotate");
    Vector3 translate = global->GetVector3Value(groupName, "Translate");

    // ドラッグで値を変更
    if (ImGui::DragFloat3("Scale", &scale.x, 0.01f)) {
        global->SetValue(groupName, "Scale", scale);
    }
    if (ImGui::DragFloat3("Rotate", &rotate.x, 0.01f)) {
        global->SetValue(groupName, "Rotate", rotate);
    }

    if (ImGui::DragFloat3("Base Translate", &translate.x, 0.1f)) {
        global->SetValue(groupName, "Translate", translate);
    }

    ImGui::Separator();

    // セーブボタン
    if (ImGui::Button("SAVE ENEMY SETTINGS (JSON)")) {
        global->SaveFile(groupName);
    }

    ImGui::End();
#endif
}

void EnemyManager::Draw(const WorldTransform& worldTransform, const ViewProjection& viewProjection, uint32_t textureHandle) {
    for (Enemy* enemy : enemies_) {
        enemy->Draw(worldTransform, viewProjection, textureHandle);
    }
}