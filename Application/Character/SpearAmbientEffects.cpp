#include "SpearPrototype.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {
float Noise(uint32_t seed) {
  seed ^= seed >> 16;
  seed *= 0x7feb352du;
  seed ^= seed >> 15;
  seed *= 0x846ca68bu;
  seed ^= seed >> 16;
  return float(seed & 0x00ffffffu) / 16777216.0f;
}
} // namespace

void SpearPrototype::DrawAmbientEffects(EffectRenderer &renderer) {
  const auto &settings = editorSettings_.effects;
  if (!settings.ambientEnabled || settings.ambientDensity <= 0 ||
      settings.ambientBrightness <= 0)
    return;
  const float time = ambientTime_ * settings.ambientDrift;
  const float combat =
      adventure_.bossState == SpearAdventure::BossState::Dormant ? 1.0f : .4f;
  // World cells enter and leave well outside the camera. Camera-relative
  // parallax preserves a stable field rather than spawning a cloud on screen.
  constexpr float span = 32;
  for (int layer = 0; layer < 3; ++layer) {
    const float parallax = layer == 0 ? .24f : layer == 1 ? .52f : .88f;
    const int center = int(std::floor(adventure_.cameraX * parallax / span));
    const int count = int(std::lround(float(layer == 0   ? 20
                                            : layer == 1 ? 14
                                                         : 6) *
                                      settings.ambientDensity));
    for (int cell = center - 1; cell <= center + 1; ++cell)
      for (int i = 0; i < count; ++i) {
        const uint32_t seed = uint32_t(cell) * 73856093u +
                              uint32_t(i) * 19349663u +
                              uint32_t(layer) * 83492791u;
        const float phase = Noise(seed + 11) * 6.2831853f;
        const float life =
            Noise(seed + 41) + time * (.004f + Noise(seed + 17) * .007f);
        const float q = life - std::floor(life);
        const float envelope = std::sin(q * 3.14159265f);
        const float wave =
            std::sin(time * (.35f + Noise(seed + 23) * .4f) + phase);
        const float twinkle = .45f + .55f * wave * wave;
        const Vector3 p{
            adventure_.cameraX * (1 - parallax) + float(cell) * span +
                Noise(seed + 3) * span + std::sin(time * .18f + phase) * .6f,
            adventure_.cameraY * (1 - parallax * .2f) + (q - .5f) * 26 +
                std::sin(time * .13f + phase) * .25f,
            layer == 0   ? 8.5f
            : layer == 1 ? 4.5f
                         : 1.1f};
        const float nearPlayer =
            std::hypot(p.x - motion_.x, p.y - motion_.y) < 1.8f ? .22f : 1;
        const float alpha = envelope * envelope * twinkle * combat *
                            nearPlayer * settings.ambientBrightness;
        if (alpha < .008f)
          continue;
        const float size =
            (.028f + Noise(seed + 29) * .03f) * (layer == 2 ? 1.65f : 1);
        const Vector4 color{.3f, .7f, 1.05f, alpha * .42f};
        if ((i + layer) % 5 == 0) {
          renderer.AddCrystal(p, phase + time * .12f, size * 1.4f, color);
          renderer.AddGlow(p, {1, 0, 0}, size * 2.4f, size * 1.5f,
                           {.18f, .6f, 1.2f, alpha * .1f});
        } else {
          renderer.AddObj(EffectRenderer::ObjEffect::Dust,p,{phase,time*.15f,phase+time*.1f},{size,size,size},color);
          renderer.AddGlow(p, {.25f, 1, 0}, size * 1.6f, size * .65f, {.3f,.7f,1.05f,alpha*.13f});
          renderer.AddGlow(p, {1, 0, 0}, size * .4f, size * .3f,
                           {.85f, 1.1f, 1.35f, alpha * .4f});
        }
        // A few faint drifting streaks add motion without bright stars or
        // flares.
        if (layer == 0 && i % 7 == 0)
          renderer.AddGlow(p, {.15f, 1, 0}, size * 4, .006f,
                           {.25f, .6f, 1, alpha * .11f});
      }
  }
}
