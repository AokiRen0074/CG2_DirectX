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
    // Version 1 resources migrate without changing the existing course.
    auto legacy=SpearStage{}.Serialize();legacy["version"]=1;
    legacy.erase("bossStart");legacy.erase("arenaPlatform");
    for(auto &e:legacy["enemies"]) e.erase(5);
    const auto migrated=SpearStage::Parse(legacy);
    assert(migrated.bossStart==98 && migrated.enemies[0].blocksPath);

    SpearStage custom;
    custom.spawns.resize(1);
    assert(custom.RemovePlatform(1) && custom.arenaPlatform==6);
    assert(!custom.RemovePlatform(custom.arenaPlatform));
    custom.platforms.push_back({140,146,2});custom.spawns.push_back({143,2});
    for(int i=0;i<20;++i) {custom.enemies.push_back({float(i)+10,5,1,true,float(i)+12,false});custom.lessons.push_back("Extra");}
    custom.SetBossStart(180);for(auto &e:custom.enemies) e.blocksPath=false;custom.Validate();
    assert(custom.Arena().left==180 && custom.Arena().right==212);
    assert(SpearStage::Parse(custom.Serialize()).Serialize()==custom.Serialize());
    game.RestartStage(custom);assert(game.enemies.size()==27);
    game.player.x=143;game.player.y=3;game.player.velocityY=-.1f;game.player.grounded=false;
    for(int i=0;i<20;++i) game.Tick(0,0,false,false);
    assert(game.player.grounded && std::abs(game.player.y-2)<.001f);
    assert(game.checkpoint==1);
    game.Respawn();assert(game.player.x==143 && game.player.y==2);
    for(auto &e:game.enemies) e.alive=false;
    game.player.x=180;game.player.y=-3;game.player.grounded=true;
    game.Tick(0,0,false,false);
    assert(game.bossState==SpearAdventure::BossState::Intro && game.checkpoint==2);
    game.Respawn();assert(game.player.x==185 && game.cameraX==196 && game.bossX==201);
    game.StartBossPractice();assert(game.player.x==185);
    while(!custom.enemies.empty()) assert(custom.RemoveEnemy(0));
    custom.Validate();game.RestartStage(custom);assert(game.enemies.empty() && game.GateX()==212);
    custom.spawns.resize(1);custom.Validate();game.RestartStage(custom);
    (void)game.Lesson(); // An empty enemy list and a single start are valid.
    custom.MovePlatform(0,2,1);assert(custom.spawns[0].x==-3 && custom.spawns[0].y==-2);
    custom.Validate();

    // A relocated arena must behave identically for both phases, including hazards.
    SpearAdventure original,relocated;
    auto shifted=SpearStage{};shifted.SetBossStart(180);relocated.RestartStage(shifted);
    original.StartBossPractice();relocated.StartBossPractice();
    original.invincibility=relocated.invincibility=10000;
    for(int i=0;i<2400;++i) {
        if(i==900) {original.BeginSecondPhase();relocated.BeginSecondPhase();}
        original.Tick(0,0,false,false);relocated.Tick(0,0,false,false);
        assert(original.bossState==relocated.bossState);
        assert(std::abs(relocated.bossX-original.bossX-82)<.005f);
        assert(std::abs(relocated.cameraX-original.cameraX-82)<.005f);
        for(size_t h=0;h<original.hazards.size();++h) {
            const auto &a=original.hazards[h],&b=relocated.hazards[h];
            assert(a.life==b.life && a.warning==b.warning);
            if(a.life>0) assert(std::abs(b.x-a.x-82)<.01f);
        }
    }
    bad=SpearStage{};bad.platforms.resize(SpearStage::maxPlatforms+1,{0,1,-3});assert(rejects(bad));
    bad=SpearStage{};bad.spawns.clear();assert(rejects(bad));
    std::cout << "Stage tests passed: serialization, validation, collision, respawn, restart, "
                 "gates, lessons, clear and boss transition.\n";
}
