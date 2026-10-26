
#pragma once
#include <memory>
#include "ViewProjection.h"
#include "WorldTransform.h"
#include "Object3d.h" 


class AxisIndicator {
private:
    // シングルトンの設定
    AxisIndicator() = default;
    ~AxisIndicator();
    AxisIndicator(const AxisIndicator&) = delete;
    const AxisIndicator& operator=(const AxisIndicator&) = delete;

public:
    static AxisIndicator* GetInstance();

    void Initialize();
    void Finalize();
    void Update();
    void Draw();

    void SetVisible(bool visible) { isVisible_ = visible; }
    void SetTargetCamera(ViewProjection* camera) { targetCamera_ = camera; }

private:
    bool isVisible_ = false;
    ViewProjection* targetCamera_ = nullptr;

    // 3Dモデル用
    std::unique_ptr<Object3d> object3d_;
    WorldTransform worldTransform_;
    uint32_t textureHandle_ = 0;

    // 軸専用のカメラ
    ViewProjection axisViewProjection_;
};
