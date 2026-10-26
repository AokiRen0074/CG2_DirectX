#include "SpearPrototype.h"
#include <cmath>

namespace {
void NovaSector(EffectRenderer &renderer, Vector3 center, float gap,
                float radius, Vector4 color) {
  // A true circle with the same angular gap as the damaging projectiles.
  constexpr int segments = 48;
  for (int i = 0; i < segments; ++i) {
    const float a = gap + .48f + float(i) * (6.2831853f - .96f) / segments;
    const float b = gap + .48f + float(i + 1) * (6.2831853f - .96f) / segments;
    renderer.AddBeam({center.x + std::cos(a) * radius,
                      center.y + std::sin(a) * radius, center.z},
                     {center.x + std::cos(b) * radius,
                      center.y + std::sin(b) * radius, center.z},
                     .018f, color);
  }
}
} // namespace

void SpearPrototype::DrawBossOverhaulEffects(EffectRenderer &renderer) {
  using State = SpearAdventure::BossState;
  const auto state = adventure_.bossState;
  if (state == State::Dormant || state == State::Defeated || adventure_.won)
    return;
  const Vector3 core{adventure_.bossX, adventure_.bossY, -.45f};
  const float phase = float(adventure_.frame) * .065f;
  for (const auto &hit : adventure_.bossImpacts) {
    if (hit.age < 0)
      continue;
    const float t = float(hit.age) / 32, fade = 1 - t;
    const Vector3 point{hit.x, hit.y, -.35f};
    const bool heavy = hit.kind == 6;
    if (hit.ground) {
      renderer.AddGlow(point, {1, 0, 0}, .7f + float(hit.age) * .025f, .035f,
                       {2.4f, .55f, .09f, fade * .35f});
      renderer.AddArc(point, 1.5707963f, .1f + float(hit.age) * .032f,
                      3.1415926f, .022f, {2.7f, .85f, .16f, fade * .3f});
      if (hit.kind == 3)
        renderer.AddCrystal({hit.x, hit.y + .2f, -.2f}, 1.5707963f, .35f * fade,
                            {2.7f, .8f, .15f, fade * .6f});
    } else if (heavy) {
      renderer.AddArc(point, phase, .25f + t * 2.3f, 6.1f, .035f,
                      {3, 1.3f, .3f, fade * .55f});
    }
    const int count = heavy ? 8 : 5;
    for (int i = 0; i < count; ++i) {
      const float a = float(i) * 2.399963f;
      const float distance =
          .1f + float(hit.age) * (.027f + float(i % 3) * .014f);
      Vector3 spark{hit.x + std::cos(a) * distance, hit.y, 0};
      spark.y += hit.ground ? .1f + 4 * t * (1 - t) * (1 + float(i % 3) * .25f)
                            : std::sin(a) * distance;
      renderer.AddCrystal(spark, a + float(hit.age) * .17f,
                          .04f + float(i % 2) * .025f,
                          {2.5f, .8f, .17f, fade * .6f});
      renderer.AddGlow(spark, {std::cos(a), hit.ground ? 1 : std::sin(a), 0},
                       .1f, .015f, {3, 1.5f, .4f, fade * .6f});
    }
    if (hit.age < 5) {
      const float flash = 1 - float(hit.age) / 5;
      renderer.AddGlow(point, {1, 0, 0}, heavy ? 2.0f : .8f, .045f,
                       {3, 2.1f, .9f, flash * .7f});
      renderer.AddGlow(point, {0, 1, 0}, heavy ? 1.5f : .5f, .035f,
                       {3, 1.3f, .3f, flash * .55f});
    }
  }
  if (state == State::Recovery) {
    const float breathe = .7f + .15f * std::sin(float(adventure_.frame) * .12f);
    renderer.AddGlow(core, {1, 0, 0}, .75f, .13f,
                     {2.8f, 1.5f, .5f, breathe * .2f});
    renderer.AddArc(core, -phase * .5f, .95f, 4.6f, .018f,
                    {2.3f, 1.3f, .4f, .2f});
  }
  if (state == State::SummonWarning) {
    const float charge =
        (std::clamp)(1 - float(adventure_.bossTimer) / 75, 0.0f, 1.0f);
    const int axis = adventure_.summonCount % 2;
    for (int i = 0; i < (adventure_.secondPhase ? 4 : 3); ++i) {
      const float offset = (float(i) - 1.5f) * 1.45f;
      const Vector3 portal{core.x + (axis == 0 ? offset : 0),
                           core.y - .5f + (axis == 1 ? offset : 0), -.5f};
      renderer.AddArc(portal, phase, .35f + charge * .3f, 5.4f, .025f,
                      {2.2f, .6f, .1f, .15f + charge * .3f});
      renderer.AddCrystal(portal, phase, .08f + charge * .17f,
                          {2.7f, .9f, .2f, charge * .6f});
    }
  }
  for (const auto &minion : adventure_.minions)
    if (minion.life > 390) {
      const float age = float(420 - minion.life) / 30;
      renderer.AddArc({minion.x, minion.y, -.4f}, -phase, .4f + age * .8f, 5.5f,
                      .035f, {2.7f, .85f, .16f, (1 - age) * .5f});
    }
  const bool nova = state == State::Nova, warning = state == State::NovaWarning;
  if (nova || warning) {
    const float charge =
        warning ? (std::clamp)(1 - float(adventure_.bossTimer) / 85, 0.0f, 1.0f)
                : 1;
    const float radius = 1.8f + charge * 1.1f;
    renderer.AddArc(core, phase, radius, 4.5f, .04f, {2.3f, .6f, .08f, .4f});
    renderer.AddArc(core, -phase, radius * .78f, 4.5f, .025f,
                    {2.5f, 1.3f, .3f, .35f});
    for (int i = 0; i < 12; ++i) {
      const float a = float(i) * 6.2831853f / 12 + phase;
      const float r = radius + .3f + std::sin(phase * 2 + float(i)) * .15f;
      renderer.AddCrystal(
          {core.x + std::cos(a) * r, core.y + std::sin(a) * r, core.z}, a + .6f,
          .12f + charge * .08f, {2.6f, .65f, .12f, .65f});
    }
    int upcoming = 0;
    if (nova)
      upcoming = adventure_.bossTimer > 140 ? 1 : 2;
    const bool telegraph =
        warning ||
        (nova && ((adventure_.bossTimer <= 164 && adventure_.bossTimer > 140) ||
                  (adventure_.bossTimer <= 116 && adventure_.bossTimer > 92)));
    if (telegraph) {
      const float gap = adventure_.NovaGap(upcoming);
      NovaSector(renderer, core, gap, 5.1f, {2.3f, .55f, .08f, .22f});
      for (float side : {-.48f, .48f}) {
        const float a = gap + side;
        renderer.AddBeam(
            core, {core.x + std::cos(a) * 9, core.y + std::sin(a) * 9, core.z},
            .018f, {2.3f, .8f, .2f, .3f});
      }
    }
    if (nova) {
      const int age = (188 - adventure_.bossTimer) % 48;
      if (age < 28) {
        const float gap = adventure_.NovaGap(adventure_.novaWave);
        NovaSector(renderer, core, gap, .3f + float(age) * .205f,
                   {2.8f, .7f, .13f, (1 - float(age) / 28) * .35f});
      }
      if (age < 6) {
        const float flash = 1 - float(age) / 6;
        renderer.AddGlow(core, {1, 0, 0}, 3.8f, .065f, {3, 2.2f, .8f, flash});
        renderer.AddGlow(core, {0, 1, 0}, 2.4f, .05f,
                         {3, 1.1f, .2f, flash * .7f});
      }
    }
  }
  if (adventure_.bossTrailLife > 0 && adventure_.bossTrailCount > 1) {
    std::array<Vector3, 14> path{};
    for (int i = 0; i < adventure_.bossTrailCount; ++i)
      path[size_t(i)] = {adventure_.bossTrail[size_t(i)][0],
                         adventure_.bossTrail[size_t(i)][1], .65f};
    const float fade = float(adventure_.bossTrailLife) / 20;
    renderer.AddRibbon(
        std::span(path.data(), size_t(adventure_.bossTrailCount)), .2f,
        {2.8f, .65f, .08f, fade * .65f});
    renderer.AddRibbon(
        std::span(path.data(), size_t(adventure_.bossTrailCount)), .035f,
        {3, 2, .7f, fade * .8f});
    for (int i = 3; i < adventure_.bossTrailCount; i += 4)
      renderer.AddCrystal(path[size_t(i)], adventure_.bossVisualAngle, .65f,
                          {2.1f, .4f, .06f, fade * .12f});
  }
  if (state == State::DashWarning) {
    const float dx = adventure_.dashTargetX - core.x,
                dy = adventure_.dashTargetY - core.y;
    const float len = (std::max)(.001f, std::sqrt(dx * dx + dy * dy));
    for (float side : {-1.7f, 1.7f}) {
      Vector3 a{core.x - dy / len * side, core.y + dx / len * side, 0};
      renderer.AddBeam(a, {a.x + dx, a.y + dy, 0}, .016f,
                       {2.4f, .5f, .08f, .16f});
    }
    renderer.AddLaserEmitter(
        core, std::atan2(dy, dx), adventure_.frame,
        (std::clamp)(1 - float(adventure_.bossTimer) / 75, 0.0f, 1.0f), false);
  }
  if (state == State::Fan) {
    const int interval = adventure_.secondPhase ? 28 : 40;
    const int age = (132 - adventure_.bossTimer - 1 + interval) % interval;
    if (age < 5)
      renderer.AddGlow(core,
                       {std::cos(adventure_.fanAimAngle),
                        std::sin(adventure_.fanAimAngle), 0},
                       2.3f, .09f,
                       {3, 1.3f, .25f, (1 - float(age) / 5) * .85f});
  }
  if (state == State::PhaseChange) {
    const float t = 1 - float(adventure_.bossTimer) / 90;
    for (int i = 0; i < 18; ++i) {
      const float angle = float(i) * 2.399963f + t * 4;
      const float r = 2.2f + t * (4 + float(i % 4));
      renderer.AddCrystal(
          {core.x + std::cos(angle) * r, core.y + std::sin(angle) * r, core.z},
          angle + t * 2, .09f, {2.6f, .65f, .12f, (1 - t) * .6f});
    }
  }
}
