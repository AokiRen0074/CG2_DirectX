#include "Application/Character/SpearAdventure.h"
#include "Application/Character/SpearUiLayout.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
using State = SpearAdventure::BossState;
void Check(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
SpearAdventure Arena() {
  SpearAdventure a;
  a.StartBossPractice();
  a.invincibility = 10000;
  a.bossX = a.bossTargetX = 119;
  a.bossY = a.bossTargetY = 4;
  return a;
}
int main() {
  try {
    // Stop tracking early enough to read exactly the destination used by the
    // dash.
    auto a = Arena();
    a.bossState = State::DashWarning;
    a.bossTimer = 20;
    a.player.x = 110;
    a.UpdateBoss();
    a.UpdateBoss();
    Check(a.dashAimLocked, "Dash aim must lock before the strike");
    const float lockedX = a.dashTargetX, lockedY = a.dashTargetY;
    a.player.x = 124;
    while (a.bossState == State::DashWarning)
      a.UpdateBoss();
    Check(a.bossState == State::Dash && a.dashTargetX == lockedX &&
              a.dashTargetY == lockedY,
          "Telegraph endpoint must not extend or track at launch");
    // Crossing the HP threshold must not interrupt an attack or snap its
    // position.
    a = Arena();
    a.bossState = State::DashWarning;
    a.bossTimer = 20;
    a.dashTargetX = 110;
    a.dashTargetY = -1;
    bool dash = false, recovery = false, phase = false;
    for (int f = 0; f < 160; ++f) {
      if (f == 4)
        a.bossHp = SpearAdventure::phaseThreshold;
      const auto old = a.bossState;
      float x = a.bossX, y = a.bossY, blend = a.bossChargeBlend,
            nova = a.bossNovaBlend;
      a.UpdateBoss();
      Check(std::hypot(a.bossX - x, a.bossY - y) < 1.8f,
            "Boss transition must not teleport");
      Check(std::abs(a.bossChargeBlend - blend) <= .101f &&
                std::abs(a.bossNovaBlend - nova) <= .081f,
            "Armor preparation must blend continuously");
      if (a.bossState == State::Dash)
        dash = true;
      if (a.bossState == State::Recovery)
        recovery = true;
      if (a.bossState == State::PhaseChange) {
        Check(dash && recovery && old == State::Recovery,
              "Phase transition must follow the completed attack");
        phase = true;
        break;
      }
    }
    Check(phase, "Pending second phase must eventually begin");
    // Refill targets accompany attacks; summoning alone no longer adds an idle
    // attack slot.
    for (bool second : {false, true}) {
      a = Arena();
      a.secondPhase = second;
      for (int i = 0; i < 12; ++i) {
        a.BeginBossMove();
        Check(a.nextWarning != State::SummonWarning,
              "Natural attack cycle should not contain an empty summon turn");
      }
    }
    // Reposition includes a settling beat even when the target is very close.
    a = Arena();
    a.BeginBossMove();
    a.bossX = a.bossTargetX;
    a.bossY = a.bossTargetY;
    for (int f = 0; f < 23; ++f) {
      a.UpdateBoss();
      Check(a.bossState == State::Reposition,
            "Boss must settle before preparing the next attack");
    }
    a.UpdateBoss();
    Check(a.bossState == a.nextWarning,
          "Settled boss must enter its telegraph");
    // Beam emitter, warning start and damage trace agree on the first
    // direction.
    a = Arena();
    a.bossState = State::BeamWarning;
    a.bossTimer = 1;
    a.beamAimAngle = -2;
    const auto start = a.BeamTrace(a.beamAimAngle - a.BeamSweepHalfAngle());
    a.UpdateBoss();
    const auto beam = a.BeamTrace();
    Check(std::abs(start.x2 - beam.x2) < .001f &&
              std::abs(start.y2 - beam.y2) < .001f,
          "Beam must begin on the highlighted warning ray");
    while (a.bossState == State::Beam)
      a.UpdateBoss();
    Check(a.bossState == State::Recovery && a.beamAfterglowFrames == 12,
          "Beam ends with a non-damaging afterglow");
    // Projectile endings fade visually but can never hurt during the opening.
    a = Arena();
    a.bossState = State::Fan;
    a.hazards = {};
    a.player.x = 103;
    a.player.y = -3;
    a.invincibility = 0;
    a.SpawnHazard(103, -3, 0, 0, 4);
    a.EnterBossRecovery(100);
    Check(a.hazards[0].life == 0 && a.retiringHazards[0].life == 12,
          "Attack endings must retain visual tails only");
    for (int f = 0; f < 13; ++f)
      a.Tick(0, 0, false, true);
    Check(a.hp == 6 && a.retiringHazards[0].life == 0,
          "Dissipating effects must not retain damage");
    a.Respawn();
    Check(a.beamAfterglowFrames == 0 && !a.phaseChangePending,
          "Retry resets transition state");
    // The authored tutorial has no accidental narrow gaps or repeated section
    // after completion.
    std::ifstream stageFile("Resources/Stages/tutorial-stage.json");
    nlohmann::json j;
    stageFile >> j;
    auto stage = SpearStage::Parse(j);
    Check(stage.platforms[1].right == 31.6f &&
              stage.platforms[2].left == 69.75f && stage.spawns.back().x == 72,
          "Practice floor and far-bank checkpoint must be deliberate");
    for (const auto &enemy : stage.enemies)
      Check(!enemy.blocksPath,
            "Tutorial must teach through terrain instead of artificial gates");
    std::ifstream uiFile("Resources/UI/PrismUI/layout.json");
    uiFile >> j;
    auto layout = SpearUiLayout::Parse(j);
    SpearUiContext c;
    c.tutorial = true;
    c.playerWorldX = 3;
    c.worldProjection[3][3] = 1;
    int buttons = 0;
    for (const auto &d : layout.Resolve(c))
      if (layout.elements[d.element].page == "tutorial") {
        Check(layout.elements[d.element].id == "tutorial_jump",
              "Tutorial should show only the initial button pictogram");
        ++buttons;
      }
    Check(buttons == 1, "Initial jump button hint must remain discoverable");
    c = {};
    c.refill = true;
    c.airReady = true;
    c.airStrike = true;
    c.dodge = true;
    c.pierce = true;
    for (const auto &d : layout.Resolve(c)) {
      const auto &e = layout.elements[d.element];
      Check(e.anchor != "feedback" &&
                (e.anchor != "player" || e.id == "jump_outline" ||
                 e.meter == "charges"),
            "No combat captions should cover the player");
    }
    std::cout
        << "PASS: locked dash telegraph, uninterrupted phase change, "
           "continuous armor/movement, settling beat, matching beam direction, "
           "harmless effect endings and focused tutorial lessons.\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    return 1;
  }
}
