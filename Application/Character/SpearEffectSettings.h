#pragma once
#include "externals/nlohmann/json.hpp"
#include <array>
#include <cmath>
#include <stdexcept>

// Player cosmetics only; these values never change movement or damage.
struct SpearEffectSettings {
  float trailWidth = 1, trailSpread = 1, trailTwist = 1, trailGlow = 1;
  int trailFrames = 16;
  std::array<float, 3> blue{0.08f / 2.4f, 0.65f / 2.4f, 1};
  std::array<float, 3> red{1, 0.04f / 2.4f, 0.25f / 2.4f};
  float sparkSize = 1, sparkSpeed = 1, sparkLife = 1;
  int dashSparkInterval = 2, hitSparkCount = 8;
  bool ambientEnabled = true;
  float ambientDensity = 1, ambientBrightness = 1, ambientDrift = 1;
  void Validate() const {
    const auto check = [](float v, float lo, float hi) {
      if (!std::isfinite(v) || v < lo || v > hi)
        throw std::invalid_argument("Invalid effect setting");
    };
    check(ambientDensity, 0, 2);
    check(ambientBrightness, 0, 2);
    check(ambientDrift, .1f, 3);
    check(trailWidth, .1f, 3);
    check(trailSpread, 0, 3);
    check(trailTwist, 0, 3);
    check(trailGlow, 0, 3);
    check(float(trailFrames), 4, 90);
    check(sparkSize, .1f, 4);
    check(sparkSpeed, .1f, 3);
    check(sparkLife, .25f, 3);
    check(float(dashSparkInterval), 1, 12);
    check(float(hitSparkCount), 0, 24);
    for (float v : blue)
      check(v, 0, 1);
    for (float v : red)
      check(v, 0, 1);
  }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    SpearEffectSettings, trailWidth, trailSpread, trailTwist, trailGlow,
    trailFrames, blue, red, sparkSize, sparkSpeed, sparkLife, dashSparkInterval,
    hitSparkCount, ambientEnabled, ambientDensity, ambientBrightness,
    ambientDrift)
