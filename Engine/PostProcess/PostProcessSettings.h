#pragma once
#include <cmath>
#include <stdexcept>

enum class ToneMapping { None = -1, Reinhard = 0, ACESApproximation = 1 };

// 全画面加工の設定。シェーダーに渡す定数への変換はFullscreenPassで行う。
struct PostProcessSettings {
    float time = 0; // 呼び出し側が更新する経過時間。
    float chromaticAberration = 0;
    float noiseIntensity = 0;
    ToneMapping toneMapping = ToneMapping::None;
    float radialBlur = 0;
    float centerX = 0.5f;
    float centerY = 0.5f;

    // NaN・無限大や範囲外の設定を、GPUへ渡す前に検出する。
    void Validate() const {
        const auto range = [](float v, float lo, float hi) {
            if (!std::isfinite(v) || v < lo || v > hi) {
                throw std::invalid_argument("Invalid post-process setting");
            }
        };
        if (!std::isfinite(time)) {
            throw std::invalid_argument("Invalid effect time");
        }
        range(chromaticAberration, 0, 0.05f);
        range(noiseIntensity, 0, 0.5f);
        range(radialBlur, 0, 0.25f);
        range(centerX, 0, 1);
        range(centerY, 0, 1);
        if (toneMapping != ToneMapping::None && toneMapping != ToneMapping::Reinhard &&
            toneMapping != ToneMapping::ACESApproximation) {
            throw std::invalid_argument("Invalid tone mapping mode");
        }
    }

    bool IsIdentity() const {
        return chromaticAberration == 0 && noiseIntensity == 0 && radialBlur == 0 && toneMapping == ToneMapping::None;
    }
};
