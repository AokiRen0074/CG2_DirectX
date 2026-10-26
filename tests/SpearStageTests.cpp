#include "Application/Character/SpearAdventure.h"
#include <cassert>
#include <iostream>
#include <limits>

int main() {
    SpearStage stage;
    assert(SpearStage::Parse(stage.Serialize()).Serialize() == stage.Serialize());
    const auto rejects = [](SpearStage s) {
        try {
            s.Validate();
        } catch (const std::exception &) {
            return true;
        }
        return false;
    };
    auto bad = stage;
    bad.platforms[0].right = bad.platforms[0].left;
    assert(rejects(bad));
    bad = stage;
    bad.spawns[0].x = 7;
    assert(rejects(bad));
    bad = stage;
    bad.spawns[2].x = 9;
    assert(rejects(bad));
    bad = stage;
    bad.enemies[0].gate = 13;
    assert(rejects(bad));
    bad = stage;
    bad.enemies[0].x = std::numeric_limits<float>::quiet_NaN();
    assert(rejects(bad));
    bad = stage;
    bad.platforms[7].top = 0;
    assert(rejects(bad));
    bool rejected = false;
    auto json = stage.Serialize();
    json["platforms"].erase(0);
    try {
        (void)SpearStage::Parse(json);
    } catch (const std::exception &) {
        rejected = true;
    }
    assert(rejected);

    SpearAdventure game;
    assert(game.GateX() == 18);
    stage.platforms[0].top = -2;
    stage.spawns[0].y = -2;
    stage.enemies[0].enabled = false;
    stage.enemies[1].x = 24;
    stage.lessons[1] = "Custom chain instruction";
    stage.bossEnabled = false;
    game.RestartStage(stage);
    assert(game.player.y == -2 && !game.enemies[0].alive && game.enemies[1].x == 24);
    assert(game.GateX() == 34);
    for (int i = 0; i < 10; ++i)
        game.Tick(0, 0, false, false);
    assert(game.player.grounded && game.player.y == -2);
    game.player.x = 20;
    assert(std::string(game.Lesson()) == stage.lessons[1]);
    game.enemies[1].alive = false;
    assert(game.stage.enemies[1].enabled);
    game.RestartStage(game.stage);
    assert(game.enemies[1].alive && !game.enemies[0].alive);
    game.checkpoint = 2;
    game.Respawn();
    assert(game.player.x == stage.spawns[2].x && game.player.y == stage.spawns[2].y);
    for (auto &enemy : game.enemies)
        enemy.alive = false;
    game.player.x = 98;
    game.player.y = -3;
    game.player.grounded = true;
    game.Tick(0, 0, false, false);
    assert(game.won && game.bossState == SpearAdventure::BossState::Dormant);
    game.bossHp = 0;
    game.secondPhase = true;
    stage.bossEnabled = true;
    game.RestartStage(stage);
    assert(game.bossHp == SpearAdventure::bossMaxHp && !game.secondPhase);
    for (auto &enemy : game.enemies)
        enemy.alive = false;
    game.player.x = 98;
    game.player.y = -3;
    game.player.grounded = true;
    game.Tick(0, 0, false, false);
    assert(!game.won && game.bossState == SpearAdventure::BossState::Intro);
    std::cout << "Stage tests passed: serialization, validation, collision, respawn, restart, "
                 "gates, lessons, clear and boss transition.\n";
}
