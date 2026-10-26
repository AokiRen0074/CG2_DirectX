#pragma once
#include "externals/nlohmann/json.hpp"
#include <array>
#include <cmath>
#include <stdexcept>
#include <string>

// Authoring data never contains transient combat state.
struct SpearStage {
    struct Platform {
        float left, right, top;
    };
    struct Enemy {
        float x, y;
        int kind;
        bool enabled = true;
        float gate = 98;
    };
    struct Spawn {
        float x, y;
    };
    std::array<Platform, 8> platforms{{{-8, 6, -3},
                                       {8, 19, -3},
                                       {33, 46, -3},
                                       {46, 62, -3},
                                       {52, 59, 8},
                                       {62, 80, -3},
                                       {80, 98, -3},
                                       {98, 130, -3}}};
    std::array<Enemy, 7> enemies{{{14, -2, 0, true, 18},
                                  {23, 0.2f, 1, true, 34},
                                  {29, 0.2f, 1, true, 34},
                                  {39, -2, 0, true, 46},
                                  {56, 1.4f, 2, true, 62},
                                  {56, 10.2f, 2, true, 62},
                                  {71, -2, 3, true, 80}}};
    std::array<Spawn, 6> spawns{{{-5, -3}, {10, -3}, {35, -3}, {48, -3}, {64, -3}, {83, -3}}};
    std::array<std::string, 7> lessons{
        {"ATTACK: jump, then D + Space to pierce the enemy",
         "CHAIN: use up to 3 air dashes across the gap", "CHAIN: defeat the next enemy and land",
         "Land to restore 3 air dashes", "UP: W + Space. Defeat the enemy to restore +1 dash",
         "UP +1: reach the next enemy and upper ledge",
         "DOWN: get above the armored enemy, then S + Space"}};
    bool bossEnabled = true;

    void Validate() const {
        const auto range = [](float v, float lo, float hi) {
            if (!std::isfinite(v) || v < lo || v > hi)
                throw std::invalid_argument("Coordinate outside supported range");
        };
        for (size_t i = 0; i < platforms.size(); ++i) {
            const auto &p = platforms[i];
            range(p.left, -8, i == 7 ? 130.0f : 98.0f);
            range(p.right, -7, i == 7 ? 130.0f : 98.0f);
            range(p.top, -8, 10);
            if (p.right - p.left < 1)
                throw std::invalid_argument("Platform width must be >= 1");
        }
        const auto &arena = platforms[7];
        if (arena.left != 98 || arena.right != 130 || arena.top != -3)
            throw std::invalid_argument("Boss arena floor is fixed");
        for (size_t i = 0; i < enemies.size(); ++i) {
            const auto &e = enemies[i];
            range(e.x, -6, 96);
            range(e.y, -7, 11);
            range(e.gate, -5, 98);
            if (e.kind < 0 || e.kind > 3 || (e.enabled && e.gate < e.x + 1))
                throw std::invalid_argument("Enemy gate must be at least 1 unit beyond enemy");
            if (lessons[i].size() > 255)
                throw std::invalid_argument("Lesson too long (255 bytes max)");
        }
        for (size_t i = 0; i < spawns.size(); ++i) {
            const auto &s = spawns[i];
            range(s.x, -7, 96);
            range(s.y, -8, 10);
            if (i > 0 && s.x <= spawns[i - 1].x)
                throw std::invalid_argument("Checkpoints must be ordered from left to right");
            bool supported = false;
            for (const auto &p : platforms)
                supported |=
                    s.x >= p.left + 0.5f && s.x <= p.right - 0.5f && std::abs(s.y - p.top) < 0.01f;
            if (!supported)
                throw std::invalid_argument("Each spawn must sit on a platform (0.5 margin)");
        }
        for (const auto &e : enemies)
            if (e.enabled && e.gate <= spawns[0].x)
                throw std::invalid_argument("Enemy gates must be beyond the start");
    }

    nlohmann::json Serialize() const {
        nlohmann::json j{{"version", 1}, {"bossEnabled", bossEnabled}, {"lessons", lessons}};
        j["platforms"] = nlohmann::json::array();
        j["enemies"] = nlohmann::json::array();
        j["spawns"] = nlohmann::json::array();
        for (const auto &p : platforms)
            j["platforms"].push_back({p.left, p.right, p.top});
        for (const auto &e : enemies)
            j["enemies"].push_back({e.x, e.y, e.kind, e.enabled, e.gate});
        for (const auto &s : spawns)
            j["spawns"].push_back({s.x, s.y});
        return j;
    }
    static SpearStage Parse(const nlohmann::json &j) {
        if (j.at("version").get<int>() != 1 || j.at("platforms").size() != 8 ||
            j.at("enemies").size() != 7 || j.at("spawns").size() != 6 ||
            j.at("lessons").size() != 7)
            throw std::invalid_argument("Unsupported stage version or slot count");
        SpearStage s;
        s.bossEnabled = j.at("bossEnabled").get<bool>();
        s.lessons = j.at("lessons").get<decltype(lessons)>();
        for (size_t i = 0; i < 8; ++i) {
            const auto &a = j.at("platforms").at(i);
            s.platforms[i] = {a.at(0).get<float>(), a.at(1).get<float>(), a.at(2).get<float>()};
        }
        for (size_t i = 0; i < 7; ++i) {
            const auto &a = j.at("enemies").at(i);
            s.enemies[i] = {a.at(0).get<float>(), a.at(1).get<float>(), a.at(2).get<int>(),
                            a.at(3).get<bool>(), a.at(4).get<float>()};
        }
        for (size_t i = 0; i < 6; ++i) {
            const auto &a = j.at("spawns").at(i);
            s.spawns[i] = {a.at(0).get<float>(), a.at(1).get<float>()};
        }
        s.Validate();
        return s;
    }
};
