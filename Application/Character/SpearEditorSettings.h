#pragma once
#include "SpearMotion.h"
#include "SpearEffectSettings.h"
#include "externals/nlohmann/json.hpp"
#include <array>
#include <stdexcept>

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SpearMotionSettings, moveSpeed, groundAcceleration,
                                   groundBraking, airAcceleration, airBraking, jumpSpeed,
                                   risingGravity, apexGravity, fallingGravity, dashSpeed, diveSpeed)

// 描画倍率はゲームの座標・当たり判定と分けて保存する。
struct SpearEditorSettings {
    SpearMotionSettings movement;
    SpearEffectSettings effects;
    std::array<std::array<float, 3>, 8> scales;
    std::array<std::array<float, 3>, 8> rotations{};
    std::array<float, 3> lightDirection{-0.45f, -0.85f, 0.35f};
    std::array<float, 3> lightColor{1.0f, 1.0f, 1.0f};
    float lightIntensity = 1.5f;

    SpearEditorSettings() {
        for (auto &scale : scales) {
            scale = {1.0f, 1.0f, 1.0f};
        }
    }

    nlohmann::json Serialize() const {
        return {{"version", 1},
                {"movement", movement},
                {"effects", effects},
                {"scales", scales},
                {"rotations", rotations},
                {"lightDirection", lightDirection},
                {"lightColor", lightColor},
                {"lightIntensity", lightIntensity}};
    }

    static SpearEditorSettings Parse(const nlohmann::json &json) {
        if (json.at("version").get<int>() != 1) {
            throw std::invalid_argument("Unsupported settings version");
        }

        SpearEditorSettings result;
        if (json.contains("effects")) result.effects = json.at("effects").get<SpearEffectSettings>();
        result.effects.Validate();
        result.movement = json.at("movement").get<SpearMotionSettings>();
        result.scales = json.at("scales").get<decltype(scales)>();
        result.rotations = json.at("rotations").get<decltype(rotations)>();
        result.lightDirection = json.at("lightDirection").get<decltype(lightDirection)>();
        result.lightColor = json.at("lightColor").get<decltype(lightColor)>();
        result.lightIntensity = json.at("lightIntensity").get<float>();

        const auto check = [](float value, float minimum, float maximum) {
            if (!std::isfinite(value) || value < minimum || value > maximum) {
                throw std::invalid_argument("Settings value outside supported range");
            }
        };

        for (const auto &scale : result.scales) {
            for (float value : scale) {
                check(value, 0.05f, 5.0f);
            }
        }

        for (const auto &rotation : result.rotations) {
            for (float value : rotation) {
                check(value, -180.0f, 180.0f);
            }
        }

        for (float value : result.lightDirection) {
            check(value, -1.0f, 1.0f);
        }

        for (float value : result.lightColor) {
            check(value, 0.0f, 1.0f);
        }

        check(result.lightIntensity, 0.0f, 10.0f);
        const auto &m = result.movement;
        check(m.moveSpeed, 0.05f, 0.30f);
        check(m.groundAcceleration, 0.005f, 0.10f);
        check(m.groundBraking, 0.005f, 0.20f);
        check(m.airAcceleration, 0.005f, 0.10f);
        check(m.airBraking, 0.005f, 0.10f);
        check(m.jumpSpeed, 0.15f, 0.50f);
        check(m.risingGravity, 0.002f, 0.04f);
        check(m.apexGravity, 0.002f, 0.02f);
        check(m.fallingGravity, 0.005f, 0.06f);
        check(m.dashSpeed, 0.30f, 1.0f);
        check(m.diveSpeed, 0.30f, 1.0f);
        return result;
    }
};
