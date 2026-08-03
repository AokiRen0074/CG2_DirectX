#pragma once
#include "Rail.h"
#include "DebugCamera.h"
#include "3D/Object3d.h"
#include "ViewProjection.h"
#include "WorldTransform.h"
#include <vector>

class RailEditor {
public:
    // 初期化
    void Initialize(Rail* rail);

    // マウス操作とレイキャストによる更新
    void Update(DebugCamera* camera);

    // 制御点の描画
    void Draw(const ViewProjection& viewProjection);

private:
    Rail* rail_ = nullptr;

    // 制御点を見えるようにするためのモデルとトランスフォーム
    Object3d* pointModel_ = nullptr;
    std::vector<WorldTransform> pointTransforms_;

    // ドラッグ操作用の状態管理
    int grabbedIndex_ = -1;      // -1 は「何も掴んでいない」状態
    float grabbedDistance_ = 0.0f; // カメラから掴んだ点までの奥行きの距離
};