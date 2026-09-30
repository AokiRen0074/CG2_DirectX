#include "EnemyManager.h"

void EnemyManager::Initialize() {
    spawnTimer_ = 0;
}

void EnemyManager::Update() {
    // スポーン処理
    spawnTimer_++;
    if (spawnTimer_ >= 60) {
        Enemy* newEnemy = new Enemy();
        newEnemy->Initialize({ 15.0f, 0.0f, 0.0f }); // 画面右端から
        enemies_.push_back(newEnemy);
        spawnTimer_ = 0;
    }

    // 全ての敵を更新
    for (Enemy* enemy : enemies_) {
        enemy->Update();
    }

    // デスフラグが立った敵、または画面外に出た敵を削除
    enemies_.remove_if([](Enemy* enemy) {
        if (enemy->IsDead() || enemy->GetPosition().x < -10.0f) {
            delete enemy;
            return true;
        }
        return false;
        });
}

void EnemyManager::Draw(const WorldTransform& worldTransform, const ViewProjection& viewProjection, uint32_t textureHandle) {
    for (Enemy* enemy : enemies_) {
        enemy->Draw(worldTransform, viewProjection, textureHandle);
    }
}