#pragma once
#include "Vector4.h"
#include <array>

// シェーダーと同じ16バイト境界で、線光源と柔らかい影を渡す。
struct SceneLighting {
    struct SegmentLight {
        Vector4 startRadius{};
        Vector4 endIntensity{};
        Vector4 color{};
    };

    struct ShadowCaster {
        Vector4 centerStrength{};
        Vector4 radii{};
    };

    Vector4 cameraEnabled{};
    Vector4 ambient{0.19f, 0.23f, 0.32f, 0};
    std::array<SegmentLight, 2> lights{};
    std::array<ShadowCaster, 16> casters{};
};

static_assert(sizeof(SceneLighting::SegmentLight) == 48);
static_assert(sizeof(SceneLighting::ShadowCaster) == 32);
static_assert(sizeof(SceneLighting) == 640);
