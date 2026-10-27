#pragma once
#include "SpearMotion.h"
#include "SpearEffectSettings.h"
#include <array>
#include <cmath>
#include <algorithm>

// 描画と切り離した寿命管理。固定更新で進め、フレームレートに演出速度を依存させない。
struct SpearEffects {
    struct Point {
        float x = 0, y = 0;
    };
    struct Trail {
        std::array<Point, 12> points{};
        int duration = 16;
        int count = 0, fade = 0, attack = -1;
        float dx = 1, dy = 0;
    };
    struct Flash {
        float x = 0, y = 0, angle = 0;
        int life = 0;
        bool hit = false;
        int rank = 0;
    };
    struct Spark {
        float x = 0, y = 0, vx = 0, vy = 0;
        int life = 0, duration = 0;
        bool red = false;
    };
    struct Shard {
        float x = 0, y = 0, vx = 0, vy = 0, angle = 0, spin = 0, size = 0;
        int life = 0;
        bool red = false;
    };
    struct Pulse {
        float x = 0, y = 0;
        int life = 0;
        bool landing = false;
    };
    SpearEffectSettings settings;
    std::array<Trail, 4> trails{};
    std::array<Flash, 8> flashes{};
    std::array<Spark, 64> sparks{};
    std::array<Shard, 32> shards{};
    std::array<Pulse, 4> pulses{};
    size_t trailCursor = 0, flashCursor = 0, sparkCursor = 0;
    size_t shardCursor = 0, pulseCursor = 0;
    int movementFrame = 0;
    float kickX = 0, kickY = 0;

    void Tick() {
        kickX *= 0.72f;
        kickY *= 0.72f;
        for (auto &shard : shards) {
            if (shard.life <= 0) {
                continue;
            }

            --shard.life;
            shard.x += shard.vx;
            shard.y += shard.vy;
            shard.vx *= 0.98f;
            shard.vy -= 0.004f;
            shard.angle += shard.spin;
        }

        for (auto &pulse : pulses) {
            if (pulse.life > 0) {
                --pulse.life;
            }
        }

        for (auto &trail : trails) {
            if (trail.fade > 0) {
                --trail.fade;
            }
        }

        for (auto &flash : flashes) {
            if (flash.life > 0) {
                --flash.life;
            }
        }

        for (auto &spark : sparks) {
            if (spark.life <= 0) {
                continue;
            }

            --spark.life;
            spark.x += spark.vx;
            spark.y += spark.vy;
            spark.vx *= 0.95f;
            spark.vy *= 0.95f;
        }
    }

    void BeginDash(const SpearMotion &motion, Point origin) {
        auto &trail = trails[trailCursor++ % trails.size()];
        trail = {};
        trail.attack = motion.attackId;
        trail.dx = motion.directionX;
        trail.dy = motion.directionY;
        trail.points[trail.count++] = origin;
        flashes[flashCursor++ % flashes.size()] = {origin.x, origin.y,
                                                   std::atan2(trail.dy, trail.dx), 9, false};
    }

    void Record(const SpearMotion &motion) {
        for (auto &trail : trails) {
            if (trail.attack != motion.attackId || trail.count == 0) {
                continue;
            }

            const auto previous = trail.points[trail.count - 1];
            if (std::hypot(previous.x - motion.x, previous.y - motion.y) < 0.001f) {
                trail.fade = trail.duration = settings.trailFrames;
                break;
            }

            if (trail.count < int(trail.points.size())) {
                trail.points[trail.count++] = {motion.x, motion.y};
            }

            trail.fade = trail.duration = settings.trailFrames;
            // 毎フレーム大量に出さず、後ろへ小粒が散る。
            if (trail.count % settings.dashSparkInterval == 0) {
                const float side = trail.count % 4 == 0 ? 1.0f : -1.0f;
                SpawnSpark(motion.x - trail.dx * 0.9f, motion.y - trail.dy * 0.9f,
                           -trail.dx * 0.09f - trail.dy * side * 0.06f,
                           -trail.dy * 0.09f + trail.dx * side * 0.06f, 20);
            }

            break;
        }
    }

    void Hit(float x, float y, float angle, int rank = 0) {
        rank = (std::clamp)(rank, 0, 4);
        flashes[flashCursor++ % flashes.size()] = {x, y, angle, 13, true, rank};
        for (int i = 0; i < settings.hitSparkCount; ++i) {
            const float direction = angle + float(i) * 2.39996f;
            const float speed = 0.08f + float(i % 3) * 0.04f;
            SpawnSpark(x, y, std::cos(direction) * speed, std::sin(direction) * speed,
                       18 + i % 3 * 4);
        }

        if (rank == 0) {
            return;
        }

        // 敵がその場で消える代わりに、攻撃方向へ割れて飛ぶ大きな結晶片を残す。
        for (int i = 0; i < 4; ++i) {
            const float spread = angle + (float(i) - 1.5f) * 0.4f;
            const float speed = 0.15f + float(i % 2) * 0.06f + float(rank) * 0.02f;
            shards[shardCursor++ % shards.size()] = {x,
                                                     y,
                                                     std::cos(spread) * speed,
                                                     std::sin(spread) * speed + 0.03f,
                                                     spread,
                                                     (i % 2 == 0 ? 1.0f : -1.0f) * 0.09f,
                                                     i == 0 ? 0.45f : 0.16f + float(i) * 0.025f,
                                                     30,
                                                     i % 2 != 0};
        }

        const float impulse = 0.045f + float(rank) * 0.012f;
        kickX = std::cos(angle) * impulse;
        kickY = std::sin(angle) * impulse;
    }

    void Movement(Point origin, bool wasGrounded, const SpearMotion &motion, bool landed) {
        ++movementFrame;
        if ((wasGrounded && !motion.grounded) || landed) {
            pulses[pulseCursor++ % pulses.size()] = {motion.x, motion.y - 0.45f, 12, landed};
        }

        const float dx = motion.x - origin.x;
        const float dy = motion.y - origin.y;
        if (!motion.IsAttacking() && movementFrame % 6 == 0 && std::hypot(dx, dy) > 0.08f) {
            SpawnSpark(motion.x - dx * 2, motion.y - 0.4f, -dx * 0.12f, 0.015f, 16,
                       movementFrame % 12 == 0);
        }
    }

    void SpawnSpark(float x, float y, float vx, float vy, int duration, bool red = false) {
        duration = (std::max)(1, int(float(duration) * settings.sparkLife));
        sparks[sparkCursor++ % sparks.size()] = {x, y, vx * settings.sparkSpeed, vy * settings.sparkSpeed, duration, duration, red};
    }
};
