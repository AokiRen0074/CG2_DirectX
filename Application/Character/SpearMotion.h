#pragma once
#include <algorithm>
#include <cmath>

// 1フレーム（60Hz）あたりの値。エディターから手触りを調整する。
struct SpearMotionSettings {
    float moveSpeed = 0.14f;
    float groundAcceleration = 0.030f;
    float groundBraking = 0.10f;
    float airAcceleration = 0.022f;
    float airBraking = 0.012f;
    float jumpSpeed = 0.32f;
    float risingGravity = 0.012f;
    float apexGravity = 0.006f;
    float fallingGravity = 0.018f;
    float dashSpeed = 0.68f;
    float diveSpeed = 0.62f; // 以前の設定ファイルとの互換用。全方向でdashSpeedを使う。
};

// 描画・入力機器から独立した、60Hz固定更新の槍アクション。
class SpearMotion {
public:
    static constexpr float groundY = -3.0f;
    // 横スクロールでは地面・足場の判定をステージ側が担当する。
    bool automaticFloor = true;
    float minimumX = -11.5f;
    float maximumX = 11.5f;
    float ceilingY = 7.0f;
    SpearMotionSettings settings;
    float x = -5.0f;
    float y = groundY;
    float velocityY = 0.0f;
    float velocityX = 0.0f;
    // 矢印の向きと突進方向は分ける。突進中の入力で軌道は曲げない。
    float aimX = 1.0f;
    float aimY = 0.0f;
    float directionX = 1.0f;
    float directionY = 0.0f;
    int charges = 3;
    int attackId = 0;
    int dashFrames = 0;
    bool grounded = true;
    bool diving = false;
    bool landed = false;

    bool IsAttacking() const {
        return dashFrames > 0;
    }

    void Step(float inputX, float inputY, bool jumpPressed, bool jumpHeld = true) {
        landed = false;
        const float inputLength = std::sqrt(inputX * inputX + inputY * inputY);

        if (inputLength > 0.01f) {
            aimX = inputX / inputLength;
            aimY = inputY / inputLength;
        }

        if (jumpPressed && grounded) {
            grounded = false;
            velocityY = settings.jumpSpeed;
        } else if (jumpPressed && charges > 0) {
            const float length = std::sqrt(inputX * inputX + inputY * inputY);

            // 方向なしでは何もせず、残り回数や現在の速度も変えない。
            if (length > 0.01f) {
                directionX = inputX / length;
                directionY = inputY / length;
                --charges;
                ++attackId;
                dashFrames = 11;
                diving = directionY < -0.01f;
                velocityY = 0.0f;
                velocityX = 0.0f;
            }
        }

        if (IsAttacking()) {
            // 出始めは鋭く、終わりは少し減速して次の操作につなぐ。
            const float speedScale = dashFrames >= 9 ? 1.0f : dashFrames >= 4 ? 0.794f : 0.588f;
            // 下・上・横・斜めを同じ時間と距離にする。
            const float dashSpeed = settings.dashSpeed * speedScale;
            x += directionX * dashSpeed;
            y += directionY * dashSpeed;

            if (dashFrames > 0) {
                --dashFrames;
            }

            // 上突進の終わりに、少し浮く余韻を残す。
            if (!IsAttacking()) {
                diving = false;
                velocityY = directionY > 0.0f ? 0.12f : 0.0f;
                velocityX = directionX * 0.18f;
            }

        } else {
            const float targetVelocity = inputX * settings.moveSpeed;
            const bool stopping = std::abs(inputX) < 0.01f;
            const bool reversing = inputX * velocityX < 0.0f;
            // 地上は停止・切り返しを素早く。空中は少し慣性を残す。
            const float acceleration =
                grounded
                    ? (stopping || reversing ? settings.groundBraking : settings.groundAcceleration)
                    : (stopping ? settings.airBraking : settings.airAcceleration);
            velocityX += (std::clamp)(targetVelocity - velocityX, -acceleration, acceleration);
            x += velocityX;

            if (std::abs(inputX) > 0.01f) {
                directionX = aimX;
                directionY = aimY;
            }

            if (!grounded) {
                // 短押しは低いジャンプ。頂点は少し浮き、落下はきびきび。
                if (!jumpHeld && velocityY > 0.12f) {
                    velocityY = 0.12f;
                }

                const float gravity = std::abs(velocityY) < 0.045f ? settings.apexGravity
                                      : velocityY < 0.0f           ? settings.fallingGravity
                                                                   : settings.risingGravity;
                velocityY -= gravity;
                y += velocityY;
            }
        }

        x = (std::clamp)(x, minimumX, maximumX);

        if (y > ceilingY) {
            y = ceilingY;
            dashFrames = 0;
            diving = false;
            velocityY = 0.0f;
        }

        if (automaticFloor && y <= groundY) {
            landed = !grounded;
            y = groundY;
            grounded = true;
            diving = false;
            dashFrames = 0;
            velocityY = 0.0f;
            charges = 3;
        }
    }
};

