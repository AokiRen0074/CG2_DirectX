#pragma once
#include "SpearMotion.h"
#include <array>

// ゲーム進行と当たり判定は描画から独立させ、60Hzで更新する。
class SpearAdventure {
public:
    struct Platform {
        float left, right, top;
    };
    struct Enemy {
        float x, y;
        int kind; // 0:通常、1:空中連続突進、2:上突進、3:下突進。
        bool alive = true;
        int lastAttack = -1;
    };
    struct Hazard {
        float x = 0, y = 0, vx = 0, vy = 0;
        int life = 0;
        int warning = 0;
        int kind = 0;
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
    };
    enum class BossState {
        Dormant,
        Intro,
        Reposition,
        DashWarning,
        Dash,
        Recovery,
        LaserWarning,
        Laser,
        BeamWarning,
        Beam,
        SummonWarning,
        Summon,
        Defeated
    };

    static constexpr std::array<Platform, 8> platforms{{{-8, 6, -3},
                                                        {8, 19, -3},
                                                        {33, 46, -3},
                                                        {46, 62, -3},
                                                        {52, 59, 8.0f},
                                                        {62, 80, -3},
                                                        {80, 98, -3},
                                                        {98, 130, -3}}};
    std::array<Enemy, 7> enemies{{{14, -2.0f, 0},
                                  {23, 0.2f, 1},
                                  {29, 0.2f, 1},
                                  {39, -2.0f, 0},
                                  {56, 1.4f, 2},
                                  {56, 10.2f, 2},
                                  {71, -2.0f, 3}}};
    std::array<Hazard, 12> hazards{};
    std::array<Particle, 48> particles{};
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
    BossState nextWarning = BossState::LaserWarning;
    int dashDuration = 30;
    int defeatFrames = 0;
    float cameraX = 0;
    float cameraY = 1.5f;
    float cameraVelocityX = 0, cameraVelocityY = 0;
    float cameraLookX = 2.5f, cameraLookY = 0;
    float cameraDistance = 32, cameraDistanceVelocity = 0;
    float beamAimAngle = -2.7f;
    // 床攻撃の試作は、調整画面から従来のレーザーへ戻して比較できる。
    bool scorchedFloorEnabled = true;
    int floorLaserDuration = 90;
    int beamDuration = 100;
    int beamWarningDuration = 85;
    int chargeRefillFlash = 0;
    int bossHp = 30;
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
        ConfigurePlayer();
    }

    void ConfigurePlayer() {
        player.automaticFloor = false;
        player.minimumX = -7;
        player.maximumX = 130;
        player.ceilingY = 12;
    }

    float GateX() const {
        const float gates[] = {18, 34, 34, 46, 62, 62, 80};
        for (size_t i = 0; i < enemies.size(); ++i) {
            if (enemies[i].alive) {
                return gates[i];
            }
        }

        return 130;
    }

    const char *Lesson() const {
        if (won) {
            return "VICTORY! R: play again";
        }

        if (bossState == BossState::BeamWarning || bossState == BossState::Beam) {
            return "SWEEP LASER: read the red guide, then jump or dash past the sweep";
        }

        if (bossState != BossState::Dormant) {
            if (bossState == BossState::LaserWarning || bossState == BossState::Laser) {
                return "FLOOR LASER: jump and use air dashes to stay above the floor";
            }

            if (bossState == BossState::SummonWarning || bossState == BossState::Summon) {
                return "PIERCE: line up ONE dash through the whole formation";
            }

            return bossState == BossState::Recovery
                       ? "OPENING! Up / down thrust deals extra damage"
                       : "BOSS: read the warning, dodge, then counterattack";
        }

        if (player.x < 10) {
            return "1. JUMP: hold Space to cross the small gap";
        }

        if (enemies[0].alive) {
            return "2. ATTACK: jump, then D + Space to pierce the enemy";
        }

        if (enemies[1].alive || enemies[2].alive) {
            return "3. CHAIN: use up to 3 air dashes across the long gap";
        }

        if (enemies[3].alive) {
            return "Keep moving! Land to restore 3 air dashes";
        }

        if (enemies[4].alive) {
            return "4. UP: jump, then W + Space. Defeat the sky enemy to restore +1 dash";
        }

        if (enemies[5].alive) {
            return "UP +1: use the restored dash to reach the next enemy and upper ledge";
        }

        if (enemies[6].alive) {
            return "5. DOWN: get above the armored enemy, then S + Space";
        }

        return "BOSS AHEAD: all spear techniques are ready";
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
        const float positions[] = {-5, 10, 35, 48, 64, 83, 103};
        player.x = positions[checkpoint];
        cameraX = checkpoint == 6 ? 114 : (std::clamp)(player.x + 2.5f, 0.0f, 90.0f);
        cameraY = checkpoint == 6 ? 3.3f : 1.5f;
        cameraVelocityX = cameraVelocityY = 0;
        cameraLookX = 2.5f;
        cameraLookY = 0;
        cameraDistance = checkpoint == 6 ? 37.0f : 32.0f;
        cameraDistanceVelocity = 0;
        hp = 6;
        invincibility = 90;
        chain = 0;
        airborneDashes = 0;
        hitStop = 0;
        hazards = {};
        particles = {};
        minions = {};
        defeatFrames = 0;
        chargeRefillFlash = 0;
        impacts = {};
        pierceCount = pierceFlash = dodgeFlash = dodgeSlowFrames = counterFrames = 0;
        lastDodgeAttack = -1;
        lastDodgeBossSequence = -1;
        bossDashSequence = 0;
        summonCount = 0;
        for (auto &enemy : enemies) {
            enemy.lastAttack = -1;
        }

        if (checkpoint == 6) {
            bossState = BossState::Intro;
            bossTimer = 75;
            bossHp = 30;
            bossCycle = -1;
            bossLastAttack = -1;
            bossX = 119;
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
        checkpoint = 6;
        for (auto &enemy : enemies) {
            enemy.alive = false;
        }

        Respawn();
        cameraX = 114;
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
        chain = 0;
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

    void Tick(float inputX, float inputY, bool pressed, bool held) {
        ++frame;
        landingEffect = false;
        invincibility = (std::max)(0, invincibility - 1);
        bossFlash = (std::max)(0, bossFlash - 1);
        shake = (std::max)(0, shake - 1);
        defeatFrames = (std::max)(0, defeatFrames - 1);
        chargeRefillFlash = (std::max)(0, chargeRefillFlash - 1);
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

        for (const auto &floor : platforms) {
            if (player.x >= floor.left && player.x <= floor.right &&
                std::abs(player.y - floor.top) < 0.01f) {
                supported = true;
            }
        }

        if (player.grounded && !supported) {
            player.grounded = false;
        }

        // 画面端にも余白を残し、方向矢印を見失わないようにする。
        player.maximumX = bossState == BossState::Dormant ? GateX() : 125;
        player.minimumX = bossState == BossState::Dormant ? -7.0f : 103.0f;
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

        // 足場は上からの着地だけを受け付ける。高速な下突進も区間で判定。
        if (!player.grounded && player.y <= oldY) {
            float landingY = -1000;
            for (const auto &floor : platforms) {
                if (player.x >= floor.left && player.x <= floor.right && oldY >= floor.top &&
                    player.y <= floor.top) {
                    landingY = (std::max)(landingY, floor.top);
                }
            }

            if (landingY > -999) {
                player.y = landingY;
                player.grounded = true;
                player.landed = true;
                player.diving = false;
                player.dashFrames = 0;
                player.velocityY = 0;
                player.charges = 3;
                airborneDashes = 0;
                landingEffect = true;
                Emit(player.x, player.y - 0.3f, wasAttacking ? 10 : 5, wasAttacking ? 1.7f : 1);
                if (wasAttacking) {
                    shake = 5;
                }
            }
        }

        const bool attacking = wasAttacking || player.IsAttacking() || player.attackId != oldAttack;
        if (attacking) {
            const float ox = player.directionX * 1.0f, oy = player.directionY * 1.0f;
            for (auto &enemy : enemies) {
                if (!enemy.alive || enemy.lastAttack == player.attackId ||
                    !SegmentHit(oldX + ox, oldY + oy, player.x + ox, player.y + oy, enemy.x,
                                enemy.y, 1.1f)) {
                    continue;
                }

                enemy.lastAttack = player.attackId;
                const bool correct = (enemy.kind != 2 || player.directionY > 0.45f) &&
                                     (enemy.kind != 3 || player.directionY < -0.45f);
                Emit(enemy.x, enemy.y, correct ? 12 : 4, correct ? 1.8f : 0.7f);
                if (!correct) {
                    continue;
                }

                enemy.alive = false;
                RegisterPierce(enemy.x, enemy.y);
                ++chain;
                maximumChain = (std::max)(maximumChain, chain);
                score += 100 * chain;
                // 空中で倒すと1回返す。攻撃から次の攻撃へつなげられる。
                if (!player.grounded) {
                    RestoreAirCharge();
                }

                shake = 6;
            }

            for (auto &minion : minions) {
                if (minion.life <= 0 || minion.lastAttack == player.attackId ||
                    !SegmentHit(oldX + ox, oldY + oy, player.x + ox, player.y + oy, minion.x,
                                minion.y, 1.0f)) {
                    continue;
                }

                minion.life = 0;
                RegisterPierce(minion.x, minion.y);
                minion.lastAttack = player.attackId;
                ++chain;
                maximumChain = (std::max)(maximumChain, chain);
                score += 100 * chain;
                if (!player.grounded) {
                    RestoreAirCharge();
                }

                // 雑魚への貫通では停止せず、そのまま奥の敵まで突き抜ける。
            }

            if (bossState != BossState::Dormant && bossState != BossState::Intro &&
                bossLastAttack != player.attackId &&
                SegmentHit(oldX + ox, oldY + oy, player.x + ox, player.y + oy, bossX, bossY,
                           2.0f)) {
                bossLastAttack = player.attackId;
                int damage = bossState == BossState::Recovery
                                 ? (std::abs(player.directionY) > 0.45f ? 4 : 2)
                                 : 1;
                if (counterFrames > 0) {
                    damage *= 2;
                    counterFrames = 0;
                    Emit(bossX, bossY, 20, 2.5f);
                }

                bossHp = (std::max)(0, bossHp - damage);
                score += damage * 100;
                bossFlash = 12;
                hitStop = 4;
                shake = 8;
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
                if (enemy.alive &&
                    SegmentHit(player.x, player.y, player.x, player.y, enemy.x, enemy.y, 0.75f)) {
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

        const float milestones[] = {10, 35, 48, 64, 83};
        for (int i = 0; i < 5; ++i) {
            if (player.grounded && player.x >= milestones[i] && checkpoint < i + 1) {
                checkpoint = i + 1;
                hp = 6;
                Emit(player.x, player.y, 8);
            }
        }

        if (bossState == BossState::Dormant && player.x >= 98) {
            checkpoint = 6;
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
        const float targetX = arena ? 114 : (std::clamp)(player.x + cameraLookX, 0.0f, 90.0f);
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

    BeamLine BeamTrace() const {
        const float progress = (std::clamp)(1 - float(bossTimer) / float(beamDuration), 0.0f, 1.0f);
        const float smooth = progress * progress * (3 - 2 * progress);
        return BeamTrace(beamAimAngle - 0.35f + smooth * 0.7f);
    }

    void SpawnHazard(float x, float y, float vx, float vy, int kind, int warning = 0) {
        for (auto &h : hazards) {
            if (h.life > 0) {
                continue;
            }

            h = {x, y, vx, vy, 160, warning, kind};
            return;
        }
    }

    void BeginBossMove() {
        bossCycle = (bossCycle + 1) % 4;
        bossState = BossState::Reposition;
        bossTimer = 180;

        if (bossCycle == 0) {
            nextWarning = BossState::LaserWarning;
            bossTargetX = 118;
            bossTargetY = 4.0f;
        } else if (bossCycle == 1) {
            nextWarning = BossState::DashWarning;
            bossTargetX = player.x < 114 ? 123.0f : 105.0f;
            bossTargetY = 3.2f;
        } else if (bossCycle == 2) {
            nextWarning = BossState::SummonWarning;
            bossTargetX = 114;
            bossTargetY = 4.5f;
        } else {
            nextWarning = BossState::BeamWarning;
            bossTargetX = player.x < 114 ? 120.0f : 108.0f;
            bossTargetY = 4.0f;
        }
    }

    void MoveBossSmoothly(float targetX, float targetY) {
        const float desiredX = (std::clamp)((targetX - bossX) * 0.07f, -0.18f, 0.18f);
        const float desiredY = (std::clamp)((targetY - bossY) * 0.07f, -0.18f, 0.18f);
        bossVelocityX += (desiredX - bossVelocityX) * 0.18f;
        bossVelocityY += (desiredY - bossVelocityY) * 0.18f;
        bossX += bossVelocityX;
        bossY += bossVelocityY;
    }

    void UpdateBoss() {
        if (bossState == BossState::Dormant || bossState == BossState::Defeated) {
            return;
        }

        const bool warning =
            bossState == BossState::DashWarning || bossState == BossState::LaserWarning ||
            bossState == BossState::SummonWarning || bossState == BossState::BeamWarning;
        armorOpening += ((warning ? 1.0f : 0.0f) - armorOpening) * 0.12f;
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

        if (bossState == BossState::Reposition) {
            const float dx = bossX - bossTargetX, dy = bossY - bossTargetY;
            if (dx * dx + dy * dy < 0.08f || bossTimer <= 0) {
                bossState = nextWarning;
                bossTimer = bossHp <= 15 ? 55 : 75;
                if (bossState == BossState::BeamWarning) {
                    beamAimAngle = std::atan2(player.y - bossY, player.x - bossX);
                    beamWarningDuration = bossHp <= 15 ? 65 : 85;
                    bossTimer = beamWarningDuration;
                }

                if (bossState == BossState::DashWarning) {
                    dashTargetX = player.x;
                    dashTargetY = player.y;
                }
            }

            return;
        }

        // 狙いを追う時間と固定する時間を分け、予告から回避を判断できるようにする。
        if (bossState == BossState::DashWarning && bossTimer > 18) {
            dashTargetX = player.x;
            dashTargetY = player.y;
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
            dashStartX = bossX;
            dashStartY = bossY;
            const float dx = dashTargetX - bossX, dy = dashTargetY - bossY;
            const float length = (std::max)(0.01f, std::sqrt(dx * dx + dy * dy));
            // プレイヤー位置で止まらず、その先まで通り抜ける。
            dashTargetX = (std::clamp)(dashTargetX + dx / length * 3.0f, 103.0f, 125.0f);
            dashTargetY = (std::clamp)(dashTargetY + dy / length * 3.0f, -1.8f, 7.0f);
            bossState = BossState::Dash;
            ++bossDashSequence;
            dashDuration = bossHp <= 15 ? 26 : 34;
            bossTimer = dashDuration;
            break;
        }

        case BossState::LaserWarning:
            bossState = BossState::Laser;
            bossTimer = bossHp <= 15 ? 105 : 90;
            floorLaserDuration = bossTimer;
            hazards[0] = {114, -2.6f, 0, 0, bossTimer, 0, 2};
            shake = 6;
            Emit(bossX, bossY, 10, 1.5f);
            break;
        case BossState::BeamWarning:
            bossState = BossState::Beam;
            beamDuration = bossHp <= 15 ? 90 : 110;
            bossTimer = beamDuration;
            shake = 8;
            Emit(bossX, bossY, 12, 1.6f);
            break;
        case BossState::SummonWarning:
            bossState = BossState::Summon;
            bossTimer = 100;
            formationAxis = summonCount++ % 2;
            formationX = (std::clamp)(player.x + (player.x < 114 ? 5.0f : -5.0f), 108.0f, 120.0f);
            for (size_t i = 0; i < minions.size(); ++i) {
                if (i >= size_t(bossHp <= 15 ? 4 : 3)) {
                    continue;
                }

                const float offset = (float(i) - 1.5f) * 1.45f;
                minions[i] = {bossX + (formationAxis == 0 ? offset : 0),
                              bossY - 0.5f + (formationAxis == 1 ? offset : 0),
                              0,
                              -0.02f,
                              420,
                              -1};
                Emit(minions[i].x, minions[i].y, 5);
            }

            break;
        case BossState::Dash:
        case BossState::Laser:
        case BossState::Beam:
        case BossState::Summon:
            bossState = BossState::Recovery;
            bossTimer = 100;
            break;
        default:
            break;
        }
    }

    float FloorDangerHalfWidth() const {
        if (hazards[0].life <= 0 || hazards[0].kind != 2) {
            return 0;
        }

        if (!scorchedFloorEnabled) {
            return 12;
        }

        // 衝撃が届いた床だけを危険にする。約0.15秒で予告範囲へ広がる。
        const int age = (std::max)(0, floorLaserDuration - hazards[0].life);
        return (std::min)(12.0f, float(age + 1) * 1.4f);
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
            if (h.kind == 2) {
                // レーザーは接触で消えず、決めた時間だけ地面を危険にする。
                if (h.life > 0 && std::abs(player.x - h.x) <= FloorDangerHalfWidth() &&
                    player.y < -2.1f) {
                    Hurt();
                }

                continue;
            }

            const float oldX = h.x, oldY = h.y;
            h.x += h.vx;
            h.y += h.vy;
            if (h.x < 99 || h.x > 130 || h.y < -3.5f) {
                h.life = 0;
            }

            if (h.life > 0 && SegmentHit(oldX, oldY, h.x, h.y, player.x, player.y, 0.6f)) {
                h.life = 0;
                Hurt();
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
            // 横列と縦列を交互に作り、一本の突進で貫ける間隔を保つ。
            const float offset = (float(i) - 1.5f) * 1.45f;
            const float targetX = formationX + (formationAxis == 0 ? offset : 0);
            const float targetY =
                0.6f + (formationAxis == 1 ? offset : 0) + std::sin(float(frame) * 0.035f) * 0.12f;
            enemy.vx +=
                ((std::clamp)((targetX - enemy.x) * 0.015f, -0.06f, 0.06f) - enemy.vx) * 0.12f;
            enemy.vy +=
                ((std::clamp)((targetY - enemy.y) * 0.018f, -0.05f, 0.05f) - enemy.vy) * 0.12f;
            enemy.x += enemy.vx;
            enemy.y += enemy.vy;

            if (!player.IsAttacking() &&
                SegmentHit(enemy.x, enemy.y, enemy.x, enemy.y, player.x, player.y, 0.7f)) {
                Hurt();
            }
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
