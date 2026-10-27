#pragma once
#include "SpearMotion.h"
#include "SpearStage.h"
#include <array>

// ゲーム進行と当たり判定は描画から独立させ、60Hzで更新する。
class SpearAdventure {
public:
    struct Platform {
        float left, right, top;
    };
    struct Enemy {
        float x, y;
        int kind; // Legacy resource field; regular enemies all use the same behavior.
        bool alive = true;
        int lastAttack = -1;
    };
    struct Hazard {
        float x = 0, y = 0, vx = 0, vy = 0;
        int life = 0;
        int warning = 0;
        int kind = 0;
    };
    struct BossImpact {
        float x=0,y=0;
        int age=-1, kind=3;
        bool ground=true;
    };
    struct Particle {
        float x = 0, y = 0, vx = 0, vy = 0;
        int life = 0;
        float size = 1.0f;
    };
    struct Impact {
        float x = 0, y = 0, angle = 0;
        int life = 0;
        int rank = 1;
    };
    struct Minion {
        float x = 0, y = 0, vx = 0, vy = 0;
        int life = 0;
        int lastAttack = -1;
        float targetX = 0, targetY = 0;
        bool route = false;
    };
    enum class BossState {
        Dormant,
        Intro,
        Reposition,
        DashWarning,
        Dash,
        Recovery,
        BeamWarning,
        Beam,
        SummonWarning,
        Summon,
        FanWarning,
        Fan,
        RainWarning,
        Rain,
        NovaWarning,
        Nova,
        PhaseChange,
        Defeated
    };

    SpearStage stage;
    std::vector<Enemy> enemies;
    std::array<Hazard, 96> hazards{};
    std::array<Hazard, 96> retiringHazards{}; // Visual tails only: never used for collision.
    int beamAfterglowFrames=0;
    std::array<float,4> beamAfterglow{};
    bool phaseChangePending=false, dashAimLocked=false;
    int bossRecoveryAge=0, bossRepositionAge=0;
    float bossChargeBlend=0, bossNovaBlend=0, bossHitBlend=0;
    std::array<Particle, 48> particles{};
    std::array<BossImpact,24> bossImpacts{};
    size_t bossImpactCursor=0;
    int groundImpactSerial=0;
    void EmitBossImpact(float x,float y,int kind,bool ground) {
        bossImpacts[bossImpactCursor++ % bossImpacts.size()]={x,y,0,kind,ground};
        if(ground) ++groundImpactSerial;
    }
    void TickBossImpacts() {
        for(auto &impact:bossImpacts) if(impact.age>=0 && ++impact.age>=32) impact.age=-1;
    }
    std::array<Impact, 4> impacts{};
    size_t particleCursor = 0;
    size_t impactCursor = 0;
    std::array<Minion, 4> minions{};
    SpearMotion player;
    BossState bossState = BossState::Dormant;
    float bossX = 119, bossY = 3.5f;
    float dashTargetX = 110, dashTargetY = -1;
    float bossTargetX = 119, bossTargetY = 3.5f;
    float bossVelocityX = 0, bossVelocityY = 0;
    float dashStartX = 119, dashStartY = 3.5f;
    float defeatX = 119, defeatY = 3.5f;
    float armorOpening = 0;
    BossState nextWarning = BossState::FanWarning;
    int dashDuration = 30;
    int defeatFrames = 0;
    float cameraX = 0;
    float cameraY = 1.5f;
    float cameraVelocityX = 0, cameraVelocityY = 0;
    float cameraLookX = 2.5f, cameraLookY = 0;
    float cameraDistance = 32, cameraDistanceVelocity = 0;
    float beamAimAngle = -2.7f;
    int beamDuration = 100;
    int beamWarningDuration = 85;
    int chargeRefillFlash = 0;
    static constexpr int bossMaxHp = 72;
    static constexpr int phaseThreshold = bossMaxHp / 2;
    int bossHp = bossMaxHp;
    bool secondPhase = false;
    float phaseFormBlend = 0;
    float bossVisualAngle = 0;
    float fanAimAngle = 0;
    int rainSafeLane = 3;
    int dashComboRemaining = 0;
    int rainWave = 0;
    float novaGapAngle = -2.35f;
    int novaWave = 0;
    std::array<std::array<float,2>,14> bossTrail{};
    int bossTrailCount = 0, bossTrailLife = 0;
    int bossTimer = 0;
    int bossCycle = -1;
    int bossLastAttack = -1;
    int bossFlash = 0;
    int hp = 6;
    int invincibility = 0;
    int hitStop = 0;
    int deaths = 0;
    int score = 0;
    int chain = 0;
    int maximumChain = 0;
    int airPower = 0; // Kills stored for the next boss strike, capped at three.
    int airKillSerial = 0;
    int airStrikeSerial = 0;
    int airStrikeFlash = 0;
    int attackHits = 0;
    int airborneDashes = 0;
    int checkpoint = 0;
    int frame = 0;
    int shake = 0;
    int pierceCount = 0;
    int bestPierce = 0;
    int pierceFlash = 0;
    int dodgeCount = 0;
    int dodgeFlash = 0;
    int dodgeSlowFrames = 0;
    int counterFrames = 0;
    int lastDodgeAttack = -1;
    int bossDashSequence = 0;
    int lastDodgeBossSequence = -1;
    float previousPlayerX = -5, previousPlayerY = -3;
    float formationX = 114;
    int formationAxis = 0;
    int summonCount = 0;
    bool jumped = false;
    bool tripleDashed = false;
    bool won = false;
    bool landingEffect = false;

    SpearAdventure() {
        RestartStage(stage);
    }

    void RestartStage(const SpearStage &definition) {
        definition.Validate();
        stage = definition;
        checkpoint = 0;
        bossState = BossState::Dormant;
        won = false;
        score = deaths = maximumChain = attackHits = frame = shake = 0;
        jumped = tripleDashed = false;
        bestPierce = dodgeCount = 0;
        enemies.resize(stage.enemies.size());
        for (size_t i = 0; i < enemies.size(); ++i) {
            const auto &e = stage.enemies[i];
            enemies[i] = {e.x, e.y, 0, e.enabled, -1};
        }
        Respawn();
    }

    void ConfigurePlayer() {
        player.automaticFloor = false;
        player.minimumX = stage.WorldLeft()+1;
        player.maximumX = stage.WorldRight();
        player.ceilingY = 12;
    }

    float GateX() const {
        float gate = stage.WorldRight();
        for (size_t i = 0; i < enemies.size(); ++i) {
            if (enemies[i].alive && stage.enemies[i].blocksPath) gate = (std::min)(gate, stage.enemies[i].gate);
        }
        return gate;
    }

    const char *Lesson() const {
        if (won) {
            return "VICTORY! R: play again";
        }

        if (bossState == BossState::PhaseChange) {
            return "PHASE II: armor awakened! Prepare for new patterns";
        }

        if (bossState == BossState::FanWarning || bossState == BossState::Fan) {
            return "CRYSTAL FAN: move through the gaps, then dash in to counter";
        }

        if (bossState == BossState::RainWarning || bossState == BossState::Rain) {
            return "LANCE RAIN: find the unmarked lane and dash into the gap";
        }

        if (bossState == BossState::BeamWarning || bossState == BossState::Beam) {
            return "SWEEP LASER: read the red guide, then jump or dash past the sweep";
        }

        if (bossState != BossState::Dormant) {
            if (bossState == BossState::SummonWarning || bossState == BossState::Summon) {
                return "CHAIN: break the blue crystals to restore +1 air jump";
            }

            return bossState == BossState::Recovery
                       ? "OPENING! Up / down thrust deals extra damage"
                       : "BOSS: read the warning, dodge, then counterattack";
        }

        if (stage.spawns.size()>1 && player.x < stage.spawns[1].x) return "JUMP: hold Space to cross the small gap";
        for (size_t i = 0; i < enemies.size(); ++i)
            if (enemies[i].alive) return stage.lessons[i].c_str();
        return stage.bossEnabled ? "BOSS AHEAD: all spear techniques are ready" : "Reach the finish marker";
    }

    void Emit(float x, float y, int count, float power = 1.0f) {
        for (int i = 0; i < count; ++i) {
            auto &p = particles[particleCursor++ % particles.size()];
            const float angle = float(i) * 2.39996f + float(frame % 17);
            p = {x, y, std::cos(angle) * 0.12f * power, std::sin(angle) * 0.12f * power + 0.06f,
                 20 + i % 10};
        }
    }

    void RegisterPierce(float x, float y) {
        ++pierceCount;
        bestPierce = (std::max)(bestPierce, pierceCount);
        pierceFlash = 24;
        const float angle = std::atan2(player.directionY, player.directionX);
        impacts[impactCursor++ % impacts.size()] = {x, y, angle, 10, pierceCount};
        Emit(x, y, 8 + pierceCount * 3, 1.5f);
        // 破片を進行方向と横方向へ散らし、刺して抜けた流れを見せる。
        const size_t count = size_t(8 + pierceCount * 3);
        for (size_t i = 0; i < count; ++i) {
            auto &p = particles[(particleCursor - count + i) % particles.size()];
            const float side = float(int(i % 7) - 3) * 0.055f;
            p.vx += player.directionX * 0.18f - player.directionY * side;
            p.vy += player.directionY * 0.18f + player.directionX * side;
            p.size = i % 3 == 0 ? 1.9f : 1.0f;
        }

        score += 100 * (pierceCount - 1);
        shake = (std::max)(shake, (std::min)(10, 3 + pierceCount));
    }

    void RegisterDodge() {
        if (!player.IsAttacking() || invincibility > 0 || lastDodgeAttack == player.attackId ||
            lastDodgeBossSequence == bossDashSequence) {
            return;
        }

        lastDodgeAttack = player.attackId;
        lastDodgeBossSequence = bossDashSequence;
        ++dodgeCount;
        dodgeFlash = 24;
        dodgeSlowFrames = 10;
        counterFrames = 90;
        score += 300;
        RestoreAirCharge();
        Emit(player.x, player.y, 10, 0.7f);
        impacts[impactCursor++ % impacts.size()] = {player.x, player.y, 0.785f, 14, 0};
    }

    void RegisterAirKill() {
        if (player.grounded) { score += 100; return; }
        ++airKillSerial;
        ++chain;
        airPower = (std::min)(3, airPower + 1);
        maximumChain = (std::max)(maximumChain, chain);
        score += 100 * chain;
        RestoreAirCharge();
        // Preserve the entire piercing dash, then give the player time to aim.
        player.refillFloatFrames = 14;
        player.velocityY = (std::max)(0.015f, player.velocityY);
    }

    void BreakAirChain() {
        chain = airPower = 0;
        player.refillFloatFrames = 0;
    }

    void RestoreAirCharge() {
        // 上限に達している時は「+1」を表示しない。実際の回復だけを知らせる。
        if (!player.grounded && player.charges < 3) {
            ++player.charges;
            chargeRefillFlash = 36;
        }
    }

    void Respawn() {
        const auto settings = player.settings;
        player = SpearMotion{};
        player.settings = settings;
        ConfigurePlayer();
        player.x = checkpoint == int(stage.spawns.size()) ? stage.ArenaX(103.0f) : stage.spawns[size_t(checkpoint)].x;
        player.y = checkpoint == int(stage.spawns.size()) ? -3.0f : stage.spawns[size_t(checkpoint)].y;
        cameraX = checkpoint == int(stage.spawns.size()) ? stage.ArenaX(114) : (std::clamp)(player.x + 2.5f, stage.WorldLeft()+8, (std::max)(stage.WorldLeft()+8,stage.bossStart-8));
        cameraY = checkpoint == int(stage.spawns.size()) ? 3.3f : 1.5f;
        cameraVelocityX = cameraVelocityY = 0;
        cameraLookX = 2.5f;
        cameraLookY = 0;
        cameraDistance = checkpoint == int(stage.spawns.size()) ? 37.0f : 32.0f;
        cameraDistanceVelocity = 0;
        hp = 6;
        invincibility = 90;
        BreakAirChain();
        airborneDashes = 0;
        hitStop = 0;
        hazards = {};
        retiringHazards={};beamAfterglowFrames=0;
        phaseChangePending=dashAimLocked=false;
        bossRecoveryAge=bossRepositionAge=0;
        bossChargeBlend=bossNovaBlend=bossHitBlend=0;
        bossImpacts={};bossImpactCursor=0;groundImpactSerial=0;
        particles = {};
        minions = {};
        defeatFrames = 0;
        chargeRefillFlash = airStrikeFlash = 0;
        impacts = {};
        pierceCount = pierceFlash = dodgeFlash = dodgeSlowFrames = counterFrames = 0;
        lastDodgeAttack = -1;
        lastDodgeBossSequence = -1;
        bossDashSequence = 0;
        summonCount = 0;
        const bool bossRetry = checkpoint == int(stage.spawns.size());
        for (size_t i = 0; i < enemies.size(); ++i) {
            auto &enemy = enemies[i];
            const auto &definition = stage.enemies[i];
            enemy.lastAttack = -1;
            if (bossRetry) {
                enemy.alive = false;
            } else if (definition.x >= player.x) {
                // Enemies are also traversal resources. Rebuild the section
                // ahead of this checkpoint on falls, death and manual retry.
                enemy = {definition.x, definition.y, 0, definition.enabled, -1};
            }
        }

        {
            bossState = checkpoint == int(stage.spawns.size()) ? BossState::Intro : BossState::Dormant;
            bossTimer = checkpoint == int(stage.spawns.size()) ? 75 : 0;
            bossHp = bossMaxHp;
            secondPhase = false;
            phaseFormBlend = 0;
            bossVisualAngle = 0;
            dashComboRemaining = 0;
            bossTrailCount = bossTrailLife = rainWave = novaWave = 0;
            bossCycle = -1;
            bossLastAttack = -1;
            bossX = stage.ArenaX(119);
            bossY = 3.5f;
            bossFlash = 0;
            bossTargetX = bossX;
            bossTargetY = bossY;
            bossVelocityX = bossVelocityY = 0;
            armorOpening = 0;
        }
    }

    void StartBossPractice() {
        won = false;
        checkpoint = int(stage.spawns.size());
        for (auto &enemy : enemies) {
            enemy.alive = false;
        }

        Respawn();
        cameraX = stage.ArenaX(114);
        cameraY = 3.3f;
        cameraDistance = 37;
    }

    void Hurt() {
        if (invincibility > 0 || won) {
            return;
        }

        --hp;
        invincibility = 60;
        shake = 10;
        BreakAirChain();
        Emit(player.x, player.y, 8);
        if (hp <= 0) {
            ++deaths;
            Respawn();
        }
    }

    // 移動区間を使い、高速で通過しても的への攻撃を取りこぼさない。
    static bool SegmentHit(float ax, float ay, float bx, float by, float x, float y, float radius) {
        const float dx = bx - ax, dy = by - ay;
        const float length = dx * dx + dy * dy;
        const float t = length > 0.0001f
                            ? (std::clamp)(((x - ax) * dx + (y - ay) * dy) / length, 0.0f, 1.0f)
                            : 0.0f;
        const float sx = ax + t * dx - x, sy = ay + t * dy - y;
        return sx * sx + sy * sy <= radius * radius;
    }

    // Player feet sweep against solid platform bodies. Resolve the earliest
    // contact, then retain the remaining movement along the surface.
    void ResolveSolidPlatforms(float fromX, float fromY, bool wasAttacking) {
      constexpr float radius = .28f, height = .85f, depth = 1.0f;
      float x = fromX, y = fromY, dx = player.x - fromX, dy = player.y - fromY;
      bool landed = false;
      for (int pass = 0; pass < 3; ++pass) {
        float first = 2, nx = 0, ny = 0;
        for (const auto &floor : stage.platforms) {
          const float loX = floor.left - radius, hiX = floor.right + radius;
          const float loY = floor.top - depth - height, hiY = floor.top;
          float enterX = -1e10f, exitX = 1e10f, enterY = -1e10f, exitY = 1e10f;
          if (std::abs(dx) < 1e-7f) {
            if (x <= loX + 1e-5f || x >= hiX - 1e-5f)
              continue;
          } else {
            enterX = ((dx > 0 ? loX : hiX) - x) / dx;
            exitX = ((dx > 0 ? hiX : loX) - x) / dx;
          }
          if (std::abs(dy) < 1e-7f) {
            if (y <= loY + 1e-5f || y >= hiY - 1e-5f)
              continue;
          } else {
            enterY = ((dy > 0 ? loY : hiY) - y) / dy;
            exitY = ((dy > 0 ? hiY : loY) - y) / dy;
          }
          const float enter = (std::max)(enterX, enterY),
                      exit = (std::min)(exitX, exitY);
          if (enter < -.00001f || enter > 1 || enter > exit || enter >= first)
            continue;
          first = (std::max)(0.0f, enter);
          if (enterY >= enterX) {
            nx = 0;
            ny = dy > 0 ? -1.0f : 1.0f;
          } else {
            nx = dx > 0 ? -1.0f : 1.0f;
            ny = 0;
          }
        }
        if (first > 1) {
          x += dx;
          y += dy;
          break;
        }
        x += dx * first;
        y += dy * first;
        dx *= 1 - first;
        dy *= 1 - first;
        player.dashFrames = 0;
        player.diving = false;
        if (nx != 0) {
          dx = 0;
          player.velocityX = 0;
        }
        if (ny != 0) {
          dy = 0;
          player.velocityY = 0;
          if (ny > 0)
            landed = true;
        }
      }
      player.x = x;
      player.y = y;
      bool support = false;
      for (const auto &floor : stage.platforms)
        support |= x > floor.left - radius + .0001f &&
                   x < floor.right + radius - .0001f &&
                   std::abs(y - floor.top) < .001f;
      player.grounded = support;
      if (landed) {
        player.landed = true;
        player.charges = 3;
        airborneDashes = 0;
        BreakAirChain();
        landingEffect = true;
        Emit(player.x, player.y - .3f, wasAttacking ? 10 : 5,
             wasAttacking ? 1.7f : 1);
        if (wasAttacking)
          shake = 5;
      }
    }

    void Tick(float inputX, float inputY, bool pressed, bool held) {
      ++frame;
      TickBossImpacts();
      beamAfterglowFrames=(std::max)(0,beamAfterglowFrames-1);
      for(auto &tail:retiringHazards) if(tail.life>0) {
        --tail.life;tail.x+=tail.vx;tail.y+=tail.vy;tail.vx*=.92f;tail.vy*=.92f;
      }
      landingEffect = false;
      invincibility = (std::max)(0, invincibility - 1);
      bossFlash = (std::max)(0, bossFlash - 1);
      shake = (std::max)(0, shake - 1);
      defeatFrames = (std::max)(0, defeatFrames - 1);
      chargeRefillFlash = (std::max)(0, chargeRefillFlash - 1);
      airStrikeFlash = (std::max)(0, airStrikeFlash - 1);
      pierceFlash = (std::max)(0, pierceFlash - 1);
      dodgeFlash = (std::max)(0, dodgeFlash - 1);
      counterFrames = (std::max)(0, counterFrames - 1);
      dodgeSlowFrames = (std::max)(0, dodgeSlowFrames - 1);
      for (auto &impact : impacts) {
        impact.life = (std::max)(0, impact.life - 1);
      }

      for (auto &p : particles) {
        if (p.life <= 0) {
          continue;
        }

        --p.life;
        p.x += p.vx;
        p.y += p.vy;
        p.vy -= 0.006f;
      }

      if (won) {
        return;
      }

      const float oldX = player.x, oldY = player.y;
      previousPlayerX = oldX;
      previousPlayerY = oldY;
      const int oldAttack = player.attackId;
      const bool wasAttacking = player.IsAttacking();
      bool supported = false;

      for (const auto &floor : stage.platforms) {
        if (player.x > floor.left - .2799f && player.x < floor.right + .2799f &&
            std::abs(player.y - floor.top) < 0.01f) {
          supported = true;
        }
      }

      if (player.grounded && !supported) {
        player.grounded = false;
      }

      // 画面端にも余白を残し、方向矢印を見失わないようにする。
      player.maximumX =
          bossState == BossState::Dormant ? GateX() : stage.ArenaX(125);
      player.minimumX = bossState == BossState::Dormant ? stage.WorldLeft() + 1
                                                        : stage.ArenaX(103);
      player.ceilingY = bossState == BossState::Dormant ? 12.0f : 9.0f;
      const bool wasGrounded = player.grounded;
      player.Step(inputX, inputY, pressed, held);

      if (wasGrounded && !player.grounded && pressed) {
        jumped = true;
        Emit(oldX, oldY - 0.4f, 5);
      }

      if (player.attackId != oldAttack) {
        pierceCount = 0;
        ++airborneDashes;
        tripleDashed |= airborneDashes >= 3;
        Emit(player.x, player.y, 5);
      }

      ResolveSolidPlatforms(oldX, oldY, wasAttacking);

      const bool attacking =
          wasAttacking || player.IsAttacking() || player.attackId != oldAttack;
      if (attacking) {
        const float ox = player.directionX * 1.0f,
                    oy = player.directionY * 1.0f;
        for (auto &enemy : enemies) {
          if (!enemy.alive || enemy.lastAttack == player.attackId ||
              !SegmentHit(oldX + ox, oldY + oy, player.x + ox, player.y + oy,
                          enemy.x, enemy.y, 1.1f)) {
            continue;
          }

          enemy.lastAttack = player.attackId;
          Emit(enemy.x, enemy.y, 12, 1.8f);
          enemy.alive = false;
          ++attackHits;
          RegisterPierce(enemy.x, enemy.y);
          RegisterAirKill();

          shake = 6;
        }

        for (auto &minion : minions) {
          if (minion.life <= 0 || minion.lastAttack == player.attackId ||
              !SegmentHit(oldX + ox, oldY + oy, player.x + ox, player.y + oy,
                          minion.x, minion.y, 1.0f)) {
            continue;
          }

          minion.life = 0;
          ++attackHits;
          RegisterPierce(minion.x, minion.y);
          minion.lastAttack = player.attackId;
          RegisterAirKill();

          // 雑魚への貫通では停止せず、そのまま奥の敵まで突き抜ける。
        }

        if (bossState != BossState::Dormant && bossState != BossState::Intro &&
            bossState != BossState::PhaseChange &&
            bossState != BossState::Defeated &&
            bossLastAttack != player.attackId &&
            SegmentHit(oldX + ox, oldY + oy, player.x + ox, player.y + oy,
                       bossX, bossY, 2.0f)) {
          bossLastAttack = player.attackId;
          ++attackHits;
          int damage = bossState == BossState::Recovery
                           ? (std::abs(player.directionY) > 0.45f ? 4 : 2)
                           : 1;
          if (counterFrames > 0) {
            damage *= 2;
            counterFrames = 0;
            Emit(bossX, bossY, 20, 2.5f);
          }

          if (!player.grounded && airPower >= 2) {
            // Cash in the route: two/three kills add four/six damage even through armor.
            damage += airPower * 2;
            airPower = 0;
            ++airStrikeSerial;
            airStrikeFlash = 42;
            RestoreAirCharge();
            player.refillFloatFrames = 14;
            Emit(bossX, bossY, 24, 2.8f);
          }
          bossHp = (std::max)(0, bossHp - damage);
          score += damage * 100;
          bossFlash = 12;
          const bool heavy = damage >= 4;
          hitStop = heavy ? 7 : 4;
          shake = heavy ? 10 : 8;
          if (heavy)
            EmitBossImpact(bossX, bossY, 6, false);
          Emit(bossX, bossY, 16, 2);
          if (bossHp == 0) {
            won = true;
            bossState = BossState::Defeated;
            hazards = {};
            minions = {};
            defeatFrames = 42;
            defeatX = bossX;
            defeatY = bossY;
            shake = 18;
            score += 3000;
            Emit(bossX, bossY, 24, 3);
          }
        }
      } else {
        for (const auto &enemy : enemies) {
          if (enemy.alive && SegmentHit(player.x, player.y, player.x, player.y,
                                        enemy.x, enemy.y, 0.75f)) {
            Hurt();
            break;
          }
        }
      }

      if (player.IsAttacking() && frame % 3 == 0) {
        Emit(player.x, player.y, 2, 0.5f);
      }

      if (player.y < -10) {
        ++deaths;
        Respawn();
      }

      for (int i = 0; i + 1 < int(stage.spawns.size()); ++i) {
        if (player.grounded && player.x >= stage.spawns[size_t(i + 1)].x &&
            checkpoint < i + 1) {
          checkpoint = i + 1;
          hp = 6;
          Emit(player.x, player.y, 8);
        }
      }

      if (bossState == BossState::Dormant && player.x >= stage.bossStart &&
          GateX() >= stage.bossStart) {
        if (!stage.bossEnabled) {
          won = true;
          UpdateCamera();
          return;
        }
        checkpoint = int(stage.spawns.size());
        bossState = BossState::Intro;
        bossTimer = 90;
        hp = 6;
        hazards = {};
      }

      if (!won) {
        // 回避成功直後は敵だけ短く遅くし、プレイヤーの操作は止めない。
        if (dodgeSlowFrames == 0 || frame % 2 == 0) {
          UpdateBoss();
          UpdateHazards();
          UpdateMinions();
        }
      }

      UpdateCamera();
    }

    static float FollowCamera(float position, float target, float &velocity, float time) {
        // 固定60Hzの減衰ばね。目標を行き過ぎず、切り返しも滑らかにつなぐ。
        constexpr float dt = 1.0f / 60;
        const float omega = 2 / time;
        const float x = omega * dt;
        const float decay = 1 / (1 + x + 0.48f * x * x + 0.235f * x * x * x);
        const float change = (std::clamp)(position - target, -35 * time, 35 * time);
        const float adjustedTarget = position - change;
        const float impulse = (velocity + omega * change) * dt;
        velocity = (velocity - omega * impulse) * decay;
        float result = adjustedTarget + (change + impulse) * decay;
        if ((target > position) == (result > target)) {
            result = target;
            velocity = 0;
        }

        return result;
    }

    void UpdateCamera() {
        const bool arena = bossState != BossState::Dormant;
        float look = cameraLookX;
        if (player.IsAttacking() && std::abs(player.directionX) > 0.7f) {
            look = player.directionX > 0 ? 2.5f : -2.5f;
        } else if (!player.IsAttacking() && std::abs(player.velocityX) > 0.025f) {
            look = player.velocityX > 0 ? 2.5f : -2.5f;
        }

        // 上下攻撃で左右の先読みを変更せず、切り返しだけをゆっくり追う。
        cameraLookX += (look - cameraLookX) * 0.06f;
        const float targetX = arena ? stage.ArenaX(114) : (std::clamp)(player.x + cameraLookX, stage.WorldLeft()+8, (std::max)(stage.WorldLeft()+8,stage.bossStart-8));
        const float targetY = arena ? 3.3f : 1.5f + (std::clamp)(player.y - 4, 0.0f, 8.5f);
        // ボス戦は画角を固定。ジャンプ連動のズームと強制位置補正を重ねない。
        cameraX = FollowCamera(cameraX, targetX, cameraVelocityX, arena ? 0.35f : 0.20f);
        cameraY = FollowCamera(cameraY, targetY, cameraVelocityY,
                               arena               ? 0.35f
                               : targetY > cameraY ? 0.10f
                                                   : 0.16f);
        cameraDistance =
            FollowCamera(cameraDistance, arena ? 37.0f : 32.0f, cameraDistanceVelocity, 0.4f);
    }

    struct BeamLine {
        float x1, y1, x2, y2;
    };
    BeamLine BeamTrace(float angle) const {
        const float dx = std::cos(angle), dy = std::sin(angle);
        // 横・上方向は画面外まで続ける。アリーナの境界で光を切らない。
        float length = 80;
        if (dy < -0.001f) {
            length = (std::min)(length, (-3.3f - bossY) / dy);
        }

        length = (std::max)(0.0f, length);
        return {bossX, bossY, bossX + dx * length, bossY + dy * length};
    }

    float BeamSweepHalfAngle() const { return secondPhase ? .65f : .45f; }

    float BeamAngle() const {
        const float progress=(std::clamp)(1-float(bossTimer)/float(beamDuration),0.0f,1.0f);
        return beamAimAngle-BeamSweepHalfAngle()+progress*progress*(3-2*progress)*BeamSweepHalfAngle()*2;
    }
    BeamLine BeamTrace() const {
        return BeamTrace(BeamAngle());
    }

    void SpawnHazard(float x, float y, float vx, float vy, int kind, int warning = 0) {
        if (kind == 2) return;
        for (auto &h : hazards) {
            if (h.life > 0) {
                continue;
            }

            h = {x, y, vx, vy, 160, warning, kind};
            return;
        }
    }

    float RainLaneX(int lane) const {
        return stage.ArenaX(102.5f) + float(lane) * 3.3f;
    }

    void SeedRefillRoute() {
        const int count=secondPhase?4:3;
        float endX=bossX,endY=(std::clamp)(bossY-.5f,1.0f,5.0f);
        if(std::abs(endX-player.x)<7) {
            endX=player.x<stage.ArenaX(114)?stage.ArenaX(122):stage.ArenaX(106);
            endY=(std::clamp)(player.y+2.5f,1.0f,5.0f);
        }
        for(int i=0;i<count;++i) {
            auto &node=minions[size_t(i)];
            if(node.life>0) continue; // Never move a live resource out from under the player.
            const float t=float(i+1)/float(count+1);
            const float x=(std::clamp)(player.x+(endX-player.x)*t,stage.ArenaX(103.5f),stage.ArenaX(124.5f));
            const float y=(std::clamp)(player.y+(endY-player.y)*t,-.8f,5.8f);
            node={x,y,0,0,420,-1,x,y,true};
            Emit(x,y,3,.35f);
        }
    }

    void RetireHazards() {
        size_t slot=0;
        for(const auto &hazard:hazards) if(hazard.life>0 && hazard.warning==0) {
            while(slot<retiringHazards.size() && retiringHazards[slot].life>0) ++slot;
            if(slot==retiringHazards.size()) break;
            retiringHazards[slot]=hazard;retiringHazards[slot++].life=12;
        }
        if(bossState==BossState::Beam) {
            const auto line=BeamTrace();beamAfterglow={line.x1,line.y1,line.x2,line.y2};beamAfterglowFrames=12;
        }
        hazards={};
    }

    void LockDashAim() {
        const float dx=dashTargetX-bossX,dy=dashTargetY-bossY;
        const float length=(std::max)(.01f,std::sqrt(dx*dx+dy*dy));
        dashTargetX=(std::clamp)(dashTargetX+dx/length*3.0f,stage.ArenaX(103),stage.ArenaX(125));
        dashTargetY=(std::clamp)(dashTargetY+dy/length*3.0f,-1.8f,7.0f);
        dashAimLocked=true;
    }

    void BeginSecondPhase() {
        bossTrailCount = bossTrailLife = 0;
        RetireHazards();
        phaseChangePending=false;
        secondPhase = true;
        bossState = BossState::PhaseChange;
        bossTimer = 90;
        bossCycle = -1;
        dashComboRemaining = 0;
        bossTargetX = stage.ArenaX(114);
        bossTargetY = 4;
        hazards = {};
        minions = {};
        player.charges = 3;
        invincibility = (std::max)(invincibility, 100);
        shake = (std::max)(shake, 6);
        Emit(bossX, bossY, 16, 1.5f);
    }

    void BeginBossMove() {
        // 第一形態は基本技と扇状弾、第二形態は槍雨と連続突進を織り交ぜる。
        static constexpr std::array<BossState, 3> first{
            {BossState::FanWarning, BossState::DashWarning, BossState::BeamWarning}};
        static constexpr std::array<BossState, 6> second{
            {BossState::NovaWarning, BossState::RainWarning, BossState::DashWarning, BossState::FanWarning,
             BossState::BeamWarning, BossState::DashWarning}};
        bossCycle = (bossCycle + 1) % int(secondPhase ? second.size() : first.size());
        nextWarning = secondPhase ? second[size_t(bossCycle)] : first[size_t(bossCycle)];
        bossState = BossState::Reposition;
        bossRepositionAge=0;
        bossTimer = secondPhase ? 105 : 125;
        bossTargetX = stage.ArenaX(118);
        bossTargetY = 4;
        if (nextWarning == BossState::DashWarning) {
            bossTargetX = player.x < stage.ArenaX(114) ? stage.ArenaX(123) : stage.ArenaX(105);
            bossTargetY = 3.2f;
        } else if (nextWarning == BossState::SummonWarning ||
                   nextWarning == BossState::RainWarning || nextWarning == BossState::NovaWarning) {
            bossTargetX = stage.ArenaX(114);
            bossTargetY = 4.8f;
        } else if (nextWarning == BossState::FanWarning || nextWarning == BossState::BeamWarning) {
            bossTargetX = player.x < stage.ArenaX(114) ? stage.ArenaX(120) : stage.ArenaX(108);
        }
    }

    int FanCount() const { return secondPhase ? 9 : 7; }
    float FanSpacing() const { return .24f; }
    bool RainSafe(int lane, int wave) const {
        const int safe = (std::clamp)(rainSafeLane + (wave % 2), 0, 6);
        return lane == safe || lane == safe + 1;
    }
    void FireRainWave(int wave, int warning) {
        rainWave = wave;
        ++bossDashSequence;
        for (int lane=0;lane<8;++lane)
            if (!RainSafe(lane,wave)) SpawnHazard(RainLaneX(lane),10,0,-.34f,3,warning);
        shake = (std::max)(shake,3);
    }
    float NovaGap(int wave) const { return novaGapAngle + float(wave) * .42f; }
    bool InNovaGap(float angle, int wave) const {
        return std::abs(std::remainder(angle-NovaGap(wave),6.2831853f)) < .48f;
    }
    void FireNova(int wave) {
        novaWave = wave;
        ++bossDashSequence;
        constexpr int count=24;
        for (int i=0;i<count;++i) {
            const float angle=float(i)*6.2831853f/count;
            if (InNovaGap(angle,wave)) continue;
            SpawnHazard(bossX,bossY,std::cos(angle)*.205f,std::sin(angle)*.205f,5);
        }
        shake = (std::max)(shake,5);
        Emit(bossX,bossY,10,1.5f);
    }
    void EnterBossRecovery(int frames) {
        RetireHazards();
        bossState=BossState::Recovery;
        bossRecoveryAge=0;
        bossTimer=frames;
        // The last wave ends before the counterattack window begins.
        hazards={};
        SeedRefillRoute();
    }
    void FireCrystalFan(bool offsetWave) {
        const int count = FanCount();
        const float spacing = FanSpacing();
        const float speed = secondPhase ? 0.23f : 0.195f;
        ++bossDashSequence;
        for (int i = 0; i < count; ++i) {
            const float angle = fanAimAngle + (float(i) - float(count - 1) * 0.5f) * spacing +
                                (offsetWave ? spacing * 0.5f : 0);
            SpawnHazard(bossX, bossY, std::cos(angle) * speed, std::sin(angle) * speed, 4);
        }
    }

    void MoveBossSmoothly(float targetX, float targetY) {
        const float desiredX = (std::clamp)((targetX - bossX) * 0.085f, -0.26f, 0.26f);
        const float desiredY = (std::clamp)((targetY - bossY) * 0.085f, -0.23f, 0.23f);
        bossVelocityX += (desiredX - bossVelocityX) * 0.18f;
        bossVelocityY += (desiredY - bossVelocityY) * 0.18f;
        bossX += bossVelocityX;
        bossY += bossVelocityY;
    }

    void UpdateBoss() {
        if (bossState == BossState::Dormant || bossState == BossState::Defeated) {
            return;
        }

        if (!secondPhase && bossHp <= phaseThreshold && bossHp > 0 && bossState != BossState::Intro)
            phaseChangePending=true;
        if(bossState==BossState::Recovery) ++bossRecoveryAge;
        if(phaseChangePending && bossState==BossState::Recovery && bossRecoveryAge>=18)
            BeginSecondPhase();
        const bool charging=bossState==BossState::DashWarning || bossState==BossState::BeamWarning ||
            bossState==BossState::FanWarning || bossState==BossState::RainWarning ||
            bossState==BossState::NovaWarning || bossState==BossState::SummonWarning;
        bossChargeBlend+=((charging?1.0f:0.0f)-bossChargeBlend)*.10f;
        bossNovaBlend+=((bossState==BossState::NovaWarning || bossState==BossState::Nova?1.0f:0.0f)-bossNovaBlend)*.08f;
        bossHitBlend+=((bossFlash>0?1.0f:0.0f)-bossHitBlend)*.25f;

        bossTrailLife = (std::max)(0,bossTrailLife-1);
        phaseFormBlend += ((secondPhase ? 1.0f : 0.0f) - phaseFormBlend) * 0.045f;
        bossVisualAngle += (0.011f + phaseFormBlend * 0.009f) * (bossState==BossState::Recovery?.25f:1);
        if (bossState == BossState::PhaseChange) {
            armorOpening += (1 - armorOpening) * 0.045f;
            MoveBossSmoothly(bossTargetX, bossTargetY);
            if (--bossTimer <= 0) {
                BeginBossMove();
            }

            return;
        }

        const bool warning =
            bossState == BossState::DashWarning ||
            bossState == BossState::SummonWarning || bossState == BossState::BeamWarning ||
            bossState == BossState::FanWarning || bossState == BossState::RainWarning ||
            bossState == BossState::NovaWarning;
        armorOpening += ((bossState==BossState::Recovery ? 1.65f : warning ? 1.0f : 0.0f) - armorOpening) * 0.12f;
        --bossTimer;

        if (bossState == BossState::Recovery) {
            MoveBossSmoothly(bossX, -0.8f);
        } else if (bossState == BossState::Dash) {
            const float oldX = bossX, oldY = bossY;
            const float progress =
                (std::clamp)(1.0f - float(bossTimer) / float(dashDuration), 0.0f, 1.0f);
            const float eased = progress * progress * (3.0f - 2.0f * progress);
            bossX = dashStartX + (dashTargetX - dashStartX) * eased;
            bossY = dashStartY + (dashTargetY - dashStartY) * eased;
            bossVelocityX = bossVelocityY = 0;
            if (bossTrailCount < int(bossTrail.size())) ++bossTrailCount;
            for (int i=bossTrailCount-1;i>0;--i) bossTrail[size_t(i)]=bossTrail[size_t(i-1)];
            bossTrail[0]={bossX,bossY};
            bossTrailLife=20;

            const bool collision = SegmentHit(oldX, oldY, bossX, bossY, player.x, player.y, 1.7f);
            if (collision) {
                Hurt();
            } else if (SegmentHit(oldX, oldY, bossX, bossY, previousPlayerX, previousPlayerY,
                                  1.7f) ||
                       SegmentHit(oldX, oldY, bossX, bossY, player.x, player.y, 2.65f)) {
                RegisterDodge();
            }
        } else {
            MoveBossSmoothly(bossTargetX, bossTargetY);
        }

        if (bossState == BossState::Beam) {
            const auto beam = BeamTrace();
            // 描画と同じ線分を使い、残っている光と当たり判定がずれないようにする。
            if (bossTimer > 0 &&
                SegmentHit(beam.x1, beam.y1, beam.x2, beam.y2, player.x, player.y, 0.55f)) {
                Hurt();
            }
        }

        if (bossState == BossState::Fan) {
            const int elapsed=132-bossTimer;
            const int interval=secondPhase?28:40;
            if (elapsed>=1 && (elapsed-1)%interval==0 && elapsed<= (secondPhase?85:81))
                FireCrystalFan(((elapsed-1)/interval)%2!=0);
        }
        if (bossState == BossState::Rain && (bossTimer==132 || bossTimer==84))
            FireRainWave(bossTimer==132?1:2,30);
        if (bossState == BossState::Nova && (bossTimer==140 || bossTimer==92))
            FireNova(bossTimer==140?1:2);

        if (bossState == BossState::Reposition) {
            const float dx = bossX - bossTargetX, dy = bossY - bossTargetY;
            ++bossRepositionAge;
            const float speed2=bossVelocityX*bossVelocityX+bossVelocityY*bossVelocityY;
            if ((dx * dx + dy * dy < 0.08f && speed2 < .0004f && bossRepositionAge>=24) || bossTimer <= 0) {
                bossState = nextWarning;
                bossTimer = secondPhase ? 55 : 75;
                if (bossState == BossState::BeamWarning) {
                    beamAimAngle = std::atan2(player.y - bossY, player.x - bossX);
                    beamWarningDuration = secondPhase ? 65 : 85;
                    bossTimer = beamWarningDuration;
                }

                if (bossState == BossState::FanWarning) {
                    fanAimAngle = std::atan2(player.y - bossY, player.x - bossX);
                    bossTimer = secondPhase ? 60 : 75;
                }

                if (bossState == BossState::RainWarning) {
                    const int nearest =
                        (std::clamp)(int(std::lround((player.x - stage.ArenaX(102.5f)) / 3.3f)), 0, 7);
                    rainSafeLane = (std::clamp)(nearest < 4 ? nearest + 1 : nearest - 2,0,6);
                    rainWave=0;
                    bossTimer = 65;
                }

                if (bossState == BossState::NovaWarning) {
                    novaGapAngle=std::atan2(player.y-bossY,player.x-bossX);
                    novaWave=0;
                    bossTimer=85;
                }
                if(bossState==BossState::FanWarning || bossState==BossState::BeamWarning ||
                   bossState==BossState::RainWarning || bossState==BossState::NovaWarning ||
                   bossState==BossState::SummonWarning) SeedRefillRoute();
                if (bossState == BossState::DashWarning) {
                    dashTargetX = player.x;
                    dashTargetY = player.y;
                    dashComboRemaining = secondPhase ? 2 : 0;
                    dashAimLocked=false;
                }
            }

            return;
        }

        // 狙いを追う時間と固定する時間を分け、予告から回避を判断できるようにする。
        if (bossState == BossState::DashWarning) {
            if(bossTimer>18) {
                dashTargetX=player.x;dashTargetY=player.y;dashAimLocked=false;
            } else if(!dashAimLocked) LockDashAim();
        }

        if (bossTimer > 0) {
            return;
        }

        switch (bossState) {
        case BossState::Intro:
        case BossState::Recovery:
            BeginBossMove();
            break;
        case BossState::DashWarning: {
            bossTrailCount = 0;
            dashStartX = bossX;
            dashStartY = bossY;
            bossState = BossState::Dash;
            ++bossDashSequence;
            dashDuration = secondPhase ? 23 : 30;
            bossTimer = dashDuration;
            break;
        }

        case BossState::BeamWarning:
            bossState = BossState::Beam;
            beamDuration = secondPhase ? 90 : 110;
            bossTimer = beamDuration;
            shake = 8;
            Emit(bossX, bossY, 12, 1.6f);
            break;
        case BossState::FanWarning:
            bossState = BossState::Fan;
            bossTimer = 132;
            break;
        case BossState::RainWarning:
            bossState = BossState::Rain;
            bossTimer = 180;
            FireRainWave(0,0);
            break;
        case BossState::NovaWarning:
            bossState=BossState::Nova;
            bossTimer=188;
            FireNova(0);
            break;
        case BossState::SummonWarning:
            bossState = BossState::Summon;
            bossTimer = 100;
            ++summonCount;
            SeedRefillRoute();

            break;
        case BossState::Dash:
            if (dashComboRemaining > 0) {
                --dashComboRemaining;
                bossState = BossState::DashWarning;
                bossTimer = 36;
                dashAimLocked=false;
                bossTargetX = bossX;
                bossTargetY = bossY;
                dashTargetX = player.x;
                dashTargetY = player.y;
                break;
            }

            EnterBossRecovery(100);
            break;
        case BossState::Nova:
            EnterBossRecovery(130);
            break;
        case BossState::Fan:
        case BossState::Rain:
        case BossState::Beam:
        case BossState::Summon:
            EnterBossRecovery(100);
            break;
        default:
            break;
        }
    }

    void UpdateHazards() {
        for (auto &h : hazards) {
            if (h.life <= 0) {
                continue;
            }

            if (h.warning > 0) {
                --h.warning;
                continue;
            }

            --h.life;
            // Kind 2 was the removed floor attack; discard any stale instance.
            if (h.kind == 2) { h.life = 0; continue; }

            const float oldX = h.x, oldY = h.y;
            h.x += h.vx;
            h.y += h.vy;
            const auto &floor=stage.Arena();
            const float floorY=floor.top-.48f;
            const bool ground=h.kind>=3 && h.vy<0 && oldY>=floorY && h.y<=floorY &&
                h.x>=floor.left && h.x<=floor.right;
            if(ground) {
                const float t=(floorY-oldY)/(h.y-oldY);
                h.x=oldX+(h.x-oldX)*t;
                h.y=floorY;
            }
            if (h.x < stage.ArenaX(99) || h.x > stage.ArenaX(130) || (!ground && h.y < -3.5f) || h.y > 12.0f) {
                h.life = 0;
            }

            const float hitRadius = h.kind == 5 ? .38f : h.kind >= 3 ? 0.32f : 0.6f;
            if (h.life > 0 && SegmentHit(oldX, oldY, h.x, h.y, player.x, player.y, hitRadius)) {
                h.life = 0;
                if(h.kind>=3) EmitBossImpact(h.x,h.y,4,false);
                Hurt();
            } else if (h.kind >= 3 && h.life > 0 &&
                       SegmentHit(oldX, oldY, h.x, h.y, player.x, player.y, hitRadius + 0.65f)) {
                RegisterDodge();
            }
            if(ground && h.life>0) {
                EmitBossImpact(h.x,floorY,h.kind,true);
                h.life=0;
            }
        }
    }

    void UpdateMinions() {
        for (size_t i = 0; i < minions.size(); ++i) {
            auto &enemy = minions[i];
            if (enemy.life <= 0) {
                continue;
            }

            --enemy.life;
            const float offset = (float(i) - 1.5f) * 1.45f;
            const float targetX = enemy.route ? enemy.targetX : formationX + (formationAxis == 0 ? offset : 0);
            const float targetY = (enemy.route ? enemy.targetY : .6f+(formationAxis==1?offset:0)) +
                std::sin(float(frame)*.035f+float(i))*.09f;
            enemy.vx +=
                ((std::clamp)((targetX - enemy.x) * 0.015f, -0.06f, 0.06f) - enemy.vx) * 0.12f;
            enemy.vy +=
                ((std::clamp)((targetY - enemy.y) * 0.018f, -0.05f, 0.05f) - enemy.vy) * 0.12f;
            enemy.x += enemy.vx;
            enemy.y += enemy.vy;

            // Refill crystals are traversal resources, never contact hazards.

        }
    }

    static float ShakeNoise(float time, float seed) {
        const float cell = std::floor(time);
        const float fraction = time - cell;
        const float smooth = fraction * fraction * (3.0f - 2.0f * fraction);
        const auto value = [seed](float x) {
            const float hash = std::sin(x * 12.9898f + seed * 78.233f) * 43758.5453f;
            return (hash - std::floor(hash)) * 2.0f - 1.0f;
        };
        return value(cell) + (value(cell + 1) - value(cell)) * smooth;
    }
};

