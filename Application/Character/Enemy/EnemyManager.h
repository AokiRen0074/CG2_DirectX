#pragma once
#include "Enemy.h"
#include <list>

class EnemyManager {
public:

    // 初期化処理
    void Initialize();

    // 更新処理
    void Update();

    // 描画処理
    void Draw(const WorldTransform& worldTransform, const ViewProjection& viewProjection, uint32_t textureHandle);
    void DrawImGui();

    // 衝突判定のために敵リストへのアクセスを提供する
    const std::list<Enemy*>& GetEnemies() const { return enemies_; }

private:
    std::list<Enemy*> enemies_;
    int spawnTimer_ = 0;
    int spawnInterval_ = 60;   // 敵が湧く間隔
    float enemySpeed_ = 0.05f; // 敵が流れるスピード
};