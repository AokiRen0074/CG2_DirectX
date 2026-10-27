#include "Application/Character/SpearAdventure.h"
#include <cassert>
#include <iostream>

using State = SpearAdventure::BossState;
int Alive(const SpearAdventure &a) {
  int n = 0;
  for (const auto &h : a.hazards)
    n += h.life > 0;
  return n;
}
SpearAdventure Arena(bool second) {
  SpearAdventure a;
  a.StartBossPractice();
  a.secondPhase = second;
  a.bossHp =
      second ? SpearAdventure::phaseThreshold : SpearAdventure::bossMaxHp;
  a.bossX = a.bossTargetX = 114;
  a.bossY = a.bossTargetY = 4.8f;
  a.player.x = 103;
  a.player.y = -3;
  a.invincibility = 10000;
  return a;
}
int main() {
  for (bool second : {false, true}) {
    auto a = Arena(second);
    a.bossState = State::FanWarning;
    a.bossTimer = 1;
    a.fanAimAngle = 3.14f;
    a.UpdateBoss();
    assert(a.bossState == State::Fan);
    int maximum = 0;
    for (int i = 0; i < 132; ++i) {
      a.UpdateBoss();
      maximum = (std::max)(maximum, Alive(a));
    }
    assert(maximum == (second ? 36 : 21));
    assert(a.bossDashSequence == (second ? 4 : 3));
    assert(a.bossState == State::Recovery && Alive(a) == 0 &&
           a.bossTimer == 100);
  }
  {
    auto a = Arena(true);
    a.rainSafeLane = 2;
    a.FireRainWave(0, 30);
    a.invincibility = 0;
    a.player.x = a.hazards[0].x;
    a.player.y = 10;
    assert(Alive(a) == 6);
    for (const auto &h : a.hazards)
      if (h.life > 0) {
        assert(std::abs(h.x - a.RainLaneX(2)) > .01f &&
               std::abs(h.x - a.RainLaneX(3)) > .01f);
      }
    for (int i = 0; i < 30; ++i)
      a.UpdateHazards();
    assert(a.hp == 6);
    for (const auto &h : a.hazards)
      if (h.life > 0)
        assert(h.y == 10 && h.warning == 0);
    a.UpdateHazards();
    assert(a.hp == 5);
    for (const auto &h : a.hazards)
      if (h.life > 0)
        assert(h.y < 10);
    assert(a.RainSafe(3, 1) && a.RainSafe(4, 1));
  }
  {
    auto a = Arena(true);
    a.novaGapAngle = -2.2f;
    for (int wave = 0; wave < 3; ++wave) {
      a.hazards = {};
      a.FireNova(wave);
      assert(Alive(a) >= 19 && Alive(a) <= 22);
      for (const auto &h : a.hazards)
        if (h.life > 0)
          assert(!a.InNovaGap(std::atan2(h.vy, h.vx), wave));
    }
    a.hazards = {};
    a.bossState = State::NovaWarning;
    a.bossTimer = 1;
    a.UpdateBoss();
    assert(a.bossState == State::Nova && a.bossTimer == 188);
    int maxAlive = Alive(a);
    for (int i = 0; i < 188; ++i) {
      a.UpdateBoss();
      maxAlive = (std::max)(maxAlive, Alive(a));
    }
    assert(maxAlive >= 57 && maxAlive <= 66);
    assert(a.bossState == State::Recovery && a.bossTimer == 130 &&
           Alive(a) == 0);
  }
  {
    auto a = Arena(true);
    a.bossState = State::DashWarning;
    a.bossTimer = 1;
    a.dashTargetX = 105;
    a.dashTargetY = -1;
    a.dashComboRemaining = 2;
    for (int i = 0; i < 180 && a.bossState != State::Recovery; ++i) {
      a.UpdateBoss();
      assert(a.bossX >= 102.9f && a.bossX <= 125.1f && a.bossY >= -1.9f &&
             a.bossY <= 9);
    }
    assert(a.bossState == State::Recovery && a.bossDashSequence == 3);
  }
  {
    auto a = Arena(false);
    a.bossState = State::Recovery;
    a.bossHp = SpearAdventure::phaseThreshold;
    a.SpawnHazard(110, 3, 0, 0, 4);
    for(int i=0;i<18;++i) a.UpdateBoss();
    assert(a.secondPhase && a.bossState == State::PhaseChange &&
           Alive(a) == 0 && a.player.charges == 3);
    for (int i = 0; i < 91; ++i)
      a.UpdateBoss();
    assert(a.nextWarning == State::NovaWarning);
    const float wide = a.BeamSweepHalfAngle();
    a.secondPhase = false;
    assert(wide > a.BeamSweepHalfAngle());
  }
  std::cout << "Impact tests...\n";
  {
    auto a = Arena(true);
    a.SpawnHazard(118, -3.2f, 0, -.6f, 3);
    const int hp = a.hp;
    a.UpdateHazards();
    assert(Alive(a) == 0 && a.groundImpactSerial == 1 && a.hp == hp);
    const auto &impact = a.bossImpacts[0];
    assert(impact.ground &&
           std::abs(impact.y - (a.stage.platforms[7].top - .48f)) < .001f);
    for (int i = 0; i < 32; ++i)
      a.TickBossImpacts();
    assert(a.bossImpacts[0].age == -1 && a.hp == hp);
    a.EmitBossImpact(110, -3.48f, 3, true);
    a.Respawn();
    assert(a.groundImpactSerial == 0 && a.bossImpactCursor == 0 &&
           a.bossImpacts[0].age == -1);
  }
  {
    auto a = Arena(true);
    a.player.x = 118;
    a.invincibility = 0;
    a.SpawnHazard(118, -2.3f, 0, -2, 3);
    a.UpdateHazards();
    assert(a.hp == 5 && a.groundImpactSerial == 0 && a.bossImpactCursor == 1);
    assert(!a.bossImpacts[0].ground);
  }
  {
    auto a = Arena(false);
    a.bossState = State::Recovery;
    a.bossTimer = 150;
    a.player.x = a.bossX;
    a.player.y = a.bossY + 2.7f;
    a.player.grounded = false;
    a.player.charges = 3;
    a.counterFrames = 90;
    a.Tick(0, -1, true, true);
    assert(a.bossHp == SpearAdventure::bossMaxHp - 8 && a.hitStop == 7);
    assert(a.bossImpactCursor == 1 && a.bossImpacts[0].kind == 6 &&
           !a.bossImpacts[0].ground);
    for (int i = 0; i < 20; ++i)
      a.UpdateBoss();
    assert(a.armorOpening > 1.5f);
  }
  // Both phases must keep cycling without creating the removed floor attack.
  for(bool second:{false,true}) {
    auto a=Arena(second);
    for(int frame=0;frame<6000;++frame) {
      a.Tick(0,0,false,true);
      for(const auto &h:a.hazards) if(h.life>0) assert(h.kind!=2);
    }
    a.hazards={};a.SpawnHazard(a.player.x,-2.6f,0,0,2);
    assert(Alive(a)==0);
    a.invincibility=0;const int hp=a.hp;
    a.hazards[0]={a.player.x,-2.6f,0,0,90,0,2};a.UpdateHazards();
    assert(a.hazards[0].life==0 && a.hp==hp);
  }
  // Refill crystals are safe to touch, including with no jumps remaining.
  {
    auto a=Arena(false);a.invincibility=0;a.player.charges=0;
    a.minions[0]={a.player.x,a.player.y,0,0,420,-1,a.player.x,a.player.y,true};
    for(int frame=0;frame<60;++frame) a.UpdateMinions();
    assert(a.hp==6 && a.invincibility==0 && a.player.charges==0 && a.minions[0].life==360);
    a.player.y=0;a.player.x=105;a.player.grounded=false;a.player.charges=1;
    a.bossState=State::Recovery;a.bossTimer=300;
    a.minions[0]={107,0,0,0,420,-1,107,0,true};
    a.Tick(1,0,true,true);
    assert(a.minions[0].life==0 && a.player.charges==1 && a.chargeRefillFlash==36 && a.hp==6);
  }
  for(bool second:{false,true}) {
    auto a=Arena(second);a.bossX=a.bossTargetX=120;a.bossY=a.bossTargetY=4.8f;
    a.SeedRefillRoute();
    int alive=0;float previousX=a.player.x;
    for(const auto &node:a.minions) if(node.life>0) {
      ++alive;assert(node.route && node.x>previousX && node.x>=a.stage.ArenaX(103.5f) && node.x<=a.stage.ArenaX(124.5f));
      assert(node.y>=-.8f && node.y<=5.8f);previousX=node.x;
    }
    assert(alive==(second?4:3));
    const auto live=a.minions[1];a.minions[0].life=0;a.player.x=124;
    a.SeedRefillRoute();
    assert(a.minions[0].life==420 && a.minions[1].x==live.x && a.minions[1].y==live.y && a.minions[1].life==live.life);
    for(const auto attack:{State::FanWarning,State::BeamWarning,State::RainWarning,State::NovaWarning,State::SummonWarning}) {
      a.minions={};a.bossState=State::Reposition;a.nextWarning=attack;a.bossTimer=1;
      a.UpdateBoss();
      assert(a.bossState==attack && a.minions[0].life==420);
    }
    a.Respawn();for(const auto &node:a.minions) assert(node.life==0);
  }
  {
    auto a=Arena(false);a.bossX=a.bossTargetX=120;
    a.bossState=State::Intro;a.bossTimer=1000;
    a.player.y=-.8f;a.player.grounded=false;a.player.charges=1;
    a.SeedRefillRoute();
    for(int jump=0;jump<4;++jump) {
      size_t next=0;while(next<a.minions.size() && a.minions[next].life<=0) ++next;
      if(next==a.minions.size()) break;
      const float dx=a.minions[next].x-a.player.x,dy=a.minions[next].y-a.player.y;
      assert(a.player.charges>0);
      for(int frame=0;frame<11;++frame) a.Tick(dx,dy,frame==0,true);
    }
    int remaining=0;for(const auto &node:a.minions) remaining+=node.life>0;
    assert(remaining==0 && a.chain==3 && a.player.charges>0 && !a.player.grounded && a.hp==6);
  }
  {
    auto a=Arena(false);a.bossState=State::Recovery;a.bossTimer=1000;
    a.invincibility=0;a.enemies[0]={a.player.x,a.player.y,0,true,-1};
    a.Tick(0,0,false,true);assert(a.hp==5); // Ordinary enemies remain hazards.
  }
  std::cout
      << "PASS: fan waves/pool, rain safe corridor and telegraph, nova gaps "
         "and recovery, triple dash, phase transition and beam range.\n";
}
