#pragma once
#include "externals/nlohmann/json.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

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
    bool blocksPath = true;
  };
  struct Spawn {
    float x, y;
  };
  std::vector<Platform> platforms{{-8, 6, -3},  {8, 19, -3},  {33, 46, -3},
                                  {46, 62, -3}, {52, 59, 8},  {62, 80, -3},
                                  {80, 98, -3}, {98, 130, -3}};
  std::vector<Enemy> enemies{{14, -2, 0, true, 18},   {23, 0.2f, 0, true, 34},
                             {29, 0.2f, 0, true, 34}, {39, -2, 0, true, 46},
                             {56, 1.4f, 0, true, 62}, {56, 10.2f, 0, true, 62},
                             {71, -2, 0, true, 80}};
  std::vector<Spawn> spawns{{-5, -3}, {10, -3}, {35, -3},
                            {48, -3}, {64, -3}, {83, -3}};
  std::vector<std::string> lessons{"Jump with Space / A",
                                   "Defeat an enemy to restore +1 air jump",
                                   "Chain air jumps through the enemies",
                                   "Defeat an enemy to restore +1 air jump",
                                   "Defeat an enemy to restore +1 air jump",
                                   "Defeat an enemy to restore +1 air jump",
                                   "Defeat an enemy to restore +1 air jump"};
  bool bossEnabled = true;

  float bossStart = 98;
  size_t arenaPlatform = 7;
  static constexpr size_t maxPlatforms = 512, maxEnemies = 256, maxSpawns = 64;
  float ArenaX(float originalX) const { return originalX + bossStart - 98; }
  const Platform &Arena() const { return platforms.at(arenaPlatform); }
  float WorldLeft() const {
    float x = -8;
    for (const auto &p : platforms)
      x = (std::min)(x, p.left);
    return x;
  }
  float WorldRight() const {
    float x = ArenaX(130);
    for (const auto &p : platforms)
      x = (std::max)(x, p.right);
    for (const auto &e : enemies)
      if (e.enabled)
        x = (std::max)(x, (std::max)(e.gate, e.x + 2));
    return x;
  }
  void SetBossStart(float x) {
    const float delta = x - bossStart;
    bossStart = x;
    auto &p = platforms.at(arenaPlatform);
    p.left += delta;
    p.right += delta;
    for (auto &spawn : spawns)
      if (spawn.x >= p.left - delta && spawn.x <= p.right - delta &&
          std::abs(spawn.y - p.top) < .01f)
        spawn.x += delta;
  }
  bool RemovePlatform(size_t index) {
    if (index >= platforms.size() || index == arenaPlatform)
      return false;
    platforms.erase(platforms.begin() + index);
    if (index < arenaPlatform)
      --arenaPlatform;
    return true;
  }
  bool RemoveEnemy(size_t index) {
    if (index >= enemies.size())
      return false;
    enemies.erase(enemies.begin() + index);
    lessons.erase(lessons.begin() + index);
    return true;
  }
  void MovePlatform(size_t index, float dx, float dy) {
    auto &p = platforms.at(index);
    if (index == arenaPlatform) {
      SetBossStart(bossStart + dx);
      return;
    }
    for (auto &spawn : spawns)
      if (spawn.x >= p.left && spawn.x <= p.right &&
          std::abs(spawn.y - p.top) < .01f) {
        spawn.x += dx;
        spawn.y += dy;
      }
    p.left += dx;
    p.right += dx;
    p.top += dy;
  }
  void Validate() const {
    const auto range = [](float v, float lo, float hi) {
      if (!std::isfinite(v) || v < lo || v > hi)
        throw std::invalid_argument("Coordinate outside supported range");
    };
    if (platforms.empty() || platforms.size() > maxPlatforms ||
        enemies.size() > maxEnemies || spawns.empty() ||
        spawns.size() > maxSpawns || lessons.size() != enemies.size() ||
        arenaPlatform >= platforms.size())
      throw std::invalid_argument("Invalid stage object count");
    range(bossStart, -128, 2000);
    const auto &arena = Arena();
    if (std::abs(arena.left - bossStart) > .01f ||
        std::abs(arena.right - bossStart - 32) > .01f || arena.top != -3)
      throw std::invalid_argument(
          "Boss floor must follow the boss start marker");
    for (const auto &p : platforms) {
      range(p.left, -256, 2047);
      range(p.right, -255, 2048);
      range(p.top, -8, 10);
      if (p.right - p.left < 1)
        throw std::invalid_argument("Platform width must be >= 1");
    }
    for (size_t i = 0; i < enemies.size(); ++i) {
      const auto &e = enemies[i];
      range(e.x, -255, 2046);
      range(e.y, -7, 11);
      range(e.gate, -254, 2048);
      if (e.kind < 0 || e.kind > 3 ||
          (e.enabled && e.blocksPath && e.gate < e.x + 1))
        throw std::invalid_argument("Blocking enemy gate must be beyond enemy");
      if (lessons[i].size() > 255)
        throw std::invalid_argument("Lesson too long (255 bytes max)");
    }
    for (size_t i = 0; i < spawns.size(); ++i) {
      const auto &spawn = spawns[i];
      range(spawn.x, -255, 2047);
      range(spawn.y, -8, 10);
      if (i > 0 && spawn.x <= spawns[i - 1].x)
        throw std::invalid_argument(
            "Checkpoints must be ordered from left to right");
      bool supported = false;
      for (const auto &p : platforms)
        supported |= spawn.x >= p.left + .5f && spawn.x <= p.right - .5f &&
                     std::abs(spawn.y - p.top) < .01f;
      if (!supported)
        throw std::invalid_argument("Place start / checkpoints on a platform");
    }
    if (bossStart < spawns[0].x + 2)
      throw std::invalid_argument("Boss start must be beyond the player start");
    for (const auto &e : enemies)
      if (e.enabled && e.blocksPath && e.gate <= spawns[0].x)
        throw std::invalid_argument("Enemy gate is before the start");
  }

  nlohmann::json Serialize() const {
    nlohmann::json j{{"version", 2},
                     {"bossEnabled", bossEnabled},
                     {"lessons", lessons},
                     {"bossStart", bossStart},
                     {"arenaPlatform", arenaPlatform}};
    j["platforms"] = nlohmann::json::array();
    j["enemies"] = nlohmann::json::array();
    j["spawns"] = nlohmann::json::array();
    for (const auto &p : platforms)
      j["platforms"].push_back({p.left, p.right, p.top});
    for (const auto &e : enemies)
      j["enemies"].push_back({e.x, e.y, 0, e.enabled, e.gate, e.blocksPath});
    for (const auto &s : spawns)
      j["spawns"].push_back({s.x, s.y});
    return j;
  }
  static SpearStage Parse(const nlohmann::json &j) {
    const int version = j.at("version").get<int>();
    if (version != 1 && version != 2)
      throw std::invalid_argument("Unsupported stage version");
    if (!j.at("platforms").is_array() || j.at("platforms").empty() ||
        j.at("platforms").size() > maxPlatforms ||
        !j.at("enemies").is_array() || j.at("enemies").size() > maxEnemies ||
        !j.at("spawns").is_array() || j.at("spawns").empty() ||
        j.at("spawns").size() > maxSpawns ||
        j.at("lessons").size() != j.at("enemies").size())
      throw std::invalid_argument("Invalid stage arrays");
    SpearStage s;
    s.bossEnabled = j.at("bossEnabled").get<bool>();
    s.bossStart = j.value("bossStart", 98.0f);
    s.arenaPlatform = j.value("arenaPlatform", size_t(7));
    s.platforms.resize(j.at("platforms").size());
    s.enemies.resize(j.at("enemies").size());
    s.spawns.resize(j.at("spawns").size());
    s.lessons = j.at("lessons").get<decltype(lessons)>();
    for (size_t i = 0; i < s.platforms.size(); ++i) {
      const auto &a = j.at("platforms").at(i);
      s.platforms[i] = {a.at(0).get<float>(), a.at(1).get<float>(),
                        a.at(2).get<float>()};
    }
    for (size_t i = 0; i < s.enemies.size(); ++i) {
      const auto &a = j.at("enemies").at(i);
      s.enemies[i] = {
          a.at(0).get<float>(), a.at(1).get<float>(),
          a.at(2).get<int>(),   a.at(3).get<bool>(),
          a.at(4).get<float>(), a.size() > 5 ? a.at(5).get<bool>() : true};
    }
    for (size_t i = 0; i < s.spawns.size(); ++i) {
      const auto &a = j.at("spawns").at(i);
      s.spawns[i] = {a.at(0).get<float>(), a.at(1).get<float>()};
    }
    s.Validate();
    for (auto &enemy : s.enemies)
      enemy.kind = 0;
    return s;
  }
};
