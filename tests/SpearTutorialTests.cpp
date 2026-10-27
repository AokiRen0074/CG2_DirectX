#include "Application/Character/SpearAdventure.h"
#include <cassert>
#include <fstream>
#include <iostream>

SpearAdventure Quiet() {
  SpearStage s;
  s.enemies.clear();
  s.lessons.clear();
  SpearAdventure a;
  a.RestartStage(s);
  return a;
}
int main() {
  auto a = Quiet();
  a.player.x = 4.5f;
  a.Tick(1, 0, true, true);
  assert(!a.player.grounded && a.player.attackId == 0 && a.player.charges == 3);
  for (int i = 0; i < 80; ++i)
    a.Tick(1, 0, false, true);
  assert(a.player.x > 8 && a.player.grounded && a.player.y == -3);

  a = Quiet();
  a.stage.platforms.push_back({-2, 2, 0});
  a.player.x = 0;
  float highest = -3;
  for (int i = 0; i < 45; ++i) {
    a.Tick(0, 0, i == 0, true);
    highest = (std::max)(highest, a.player.y);
  }
  assert(highest <= -1.849f && a.player.grounded);
  a.player.x = 0;
  a.player.y = -3;
  a.player.grounded = false;
  for (int i = 0; i < 3; ++i)
    a.Tick(0, 1, i == 0, true);
  assert(a.player.y <= -1.849f && a.player.dashFrames == 0 &&
         a.player.charges == 2);

  a = Quiet();
  a.player.x = 0;
  a.player.y = 6;
  a.player.grounded = false;
  a.ResolveSolidPlatforms(0, -6, true);
  assert(std::abs(a.player.y + 4.85f) < .001f && !a.player.grounded);
  a.stage.platforms = {{0, 6, -3}, {98, 130, -3}};
  a.stage.arenaPlatform = 1;
  a.player.x = 5;
  a.player.y = -3.6f;
  a.player.grounded = false;
  a.ResolveSolidPlatforms(-2, -3.6f, true);
  assert(std::abs(a.player.x + .28f) < .001f);
  a.player.x = 2;
  a.player.y = -8;
  a.player.charges = 0;
  a.ResolveSolidPlatforms(2, 3, true);
  assert(a.player.y == -3 && a.player.grounded && a.player.charges == 3);
  a = Quiet();
  a.player.x = 44;
  for (int i = 0; i < 30; ++i) {
    a.Tick(1, 0, false, true);
    assert(a.player.grounded && a.player.y == -3);
  }

  for (int oldKind = 0; oldKind < 4; ++oldKind) {
    a = Quiet();
    a.stage.enemies.push_back({1, 0, oldKind, true, 3, false});
    a.stage.lessons.push_back("");
    a.RestartStage(a.stage);
    assert(a.enemies[0].kind == 0);
    a.enemies[0].kind =
        oldKind; // Legacy state cannot re-enable directional armor.
    a.player.x = -1;
    a.player.y = 0;
    a.player.grounded = false;
    a.player.charges = 1;
    a.Tick(1, 0, true, true);
    assert(!a.enemies[0].alive && a.player.charges == 1 &&
           a.chargeRefillFlash == 36);
  }

  // Traverse the user's current airborne tutorial with the actual authored
  // data.
  std::ifstream file("Resources/Stages/tutorial-stage.json");
  nlohmann::json j;
  file >> j;
  const auto stage = SpearStage::Parse(j);
  a.RestartStage(stage);
  for (auto &enemy : a.enemies)
    if (enemy.x < 31)
      enemy.alive = false;
  a.player.x = 31.5f;
  a.player.y = .5f;
  a.player.grounded = false;
  for (int dash = 0; dash < 7; ++dash)
    for (int frame = 0; frame < 11; ++frame)
      a.Tick(1, 0, frame == 0, true);
  for (const auto &enemy : a.enemies)
    if (enemy.x < 60)
      assert(!enemy.alive);
  assert(a.attackHits == 5 && a.player.x > 69.75f);
  for (int frame = 0; frame < 50; ++frame)
    a.Tick(0, 0, false, true);
  assert(a.player.grounded && a.player.y == -3 && a.player.charges == 3 &&
         a.deaths == 0);
  // Killing stepping-stone enemies and falling must never consume the route.
  a.RestartStage(stage);
  a.checkpoint=1;
  for(int attempt=0;attempt<3;++attempt) {
    for(auto &enemy:a.enemies) if(enemy.x<31) enemy.alive=false;
    a.player.x=31.5f;a.player.y=.5f;a.player.grounded=false;
    for(int dash=0;dash<2;++dash) for(int frame=0;frame<11;++frame) a.Tick(1,0,frame==0,true);
    assert(!a.enemies[3].alive && !a.enemies[4].alive);
    a.player.y=-10.5f;a.player.grounded=false;
    a.Tick(0,0,false,true);
    assert(a.deaths==attempt+1 && a.player.x==stage.spawns[1].x && a.player.charges==3);
    for(size_t enemy=0;enemy<a.enemies.size();++enemy)
      if(stage.enemies[enemy].x>=stage.spawns[1].x) assert(a.enemies[enemy].alive==stage.enemies[enemy].enabled);
  }
  // The restored enemies can actually be killed again to cross the gap.
  for(auto &enemy:a.enemies) if(enemy.x<31) enemy.alive=false;
  a.player.x=31.5f;a.player.y=.5f;a.player.grounded=false;
  const int hitsBeforeRetry=a.attackHits;
  for(int dash=0;dash<7;++dash) for(int frame=0;frame<11;++frame) a.Tick(1,0,frame==0,true);
  for(int frame=0;frame<50;++frame) a.Tick(0,0,false,true);
  assert(a.attackHits==hitsBeforeRetry+5 && a.player.x>69.75f && a.player.grounded && a.deaths==3);

  a.enemies[3].alive=false;a.hp=1;a.invincibility=0;a.Hurt();
  assert(a.hp==6 && !a.enemies[3].alive && a.player.x==stage.spawns[3].x);
  a.enemies[3].alive=false;a.Respawn();assert(!a.enemies[3].alive); // Manual retry uses this path.

  auto checkpointStage=stage;
  checkpointStage.enemies[5].enabled=false;
  a.RestartStage(checkpointStage);a.checkpoint=2;
  for(auto &enemy:a.enemies) enemy.alive=false;
  a.Respawn();
  assert(!a.enemies[0].alive && !a.enemies[1].alive && !a.enemies[2].alive);
  assert(a.enemies[3].alive && a.enemies[4].alive && !a.enemies[5].alive && a.enemies[6].alive);
  assert(a.GateX()>=a.player.x);
  a.StartBossPractice();a.Respawn();
  for(const auto &enemy:a.enemies) assert(!enemy.alive);
  // After the airborne crossing there is a quiet, unobstructed approach.
  a.RestartStage(stage);
  for(const auto &enemy:a.enemies) assert(enemy.x<80);
  for(const auto &floor:stage.platforms) assert(floor.top==-3);
  a.player.x=81;a.player.y=-3;a.player.grounded=true;
  for(int frame=0;frame<150;++frame) a.Tick(1,0,false,true);
  assert(a.bossState!=SpearAdventure::BossState::Dormant && a.deaths==0);
  std::cout
      << "PASS: normal jump, underside/side/high-speed solid contacts, seams, "
         "single enemy, +1 refill, tutorial traversal, repeated fall/death/manual retry and checkpoint/boss enemy restoration.\n";
}
