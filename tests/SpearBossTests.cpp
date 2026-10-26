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
    a.UpdateBoss();
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
  std::cout
      << "PASS: fan waves/pool, rain safe corridor and telegraph, nova gaps "
         "and recovery, triple dash, phase transition and beam range.\n";
}
