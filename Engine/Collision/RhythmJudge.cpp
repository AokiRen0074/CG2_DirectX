#include "RhythmJudge.h"
#include <cmath>

void RhythmJudge::Initialize() {}

void RhythmJudge::Update(Player* player, EnemyManager* enemyManager) {
    Input* input = Input::GetInstance();

    bool isAttackUpper = input->TriggerKey(DIK_UP) || input->TriggerKey(DIK_W);
    bool isAttackLower = input->TriggerKey(DIK_DOWN) || input->TriggerKey(DIK_S);

    Vector3 playerPos = player->GetPosition();

    for (Enemy* enemy : enemyManager->GetEnemies()) {
        if (enemy->IsDead()) continue;

        Vector3 enemyPos = enemy->GetPosition();
        bool isUpperEnemy = (enemyPos.y > 1.0f);

        // 攻撃判
        float hitRangeX = 0.2f;

        if (std::abs(playerPos.x - enemyPos.x) < hitRangeX) {
            // 上レーンの敵をWで叩いた
            if (isUpperEnemy && isAttackUpper) {
                enemy->OnCollision();
                break;
            }
            // 下レーンの敵をSで叩いた
            if (!isUpperEnemy && isAttackLower) {
                enemy->OnCollision();
                break; 
            }
        }

        // 被弾判定
        bool isSameLane = (std::abs(playerPos.y - enemyPos.y) < 1.0f);
        float bodyHitRangeX = 0.2f;

        if (isSameLane && (std::abs(playerPos.x - enemyPos.x) < bodyHitRangeX)) {
            enemy->OnCollision();
        }
    }
}