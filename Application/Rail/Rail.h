#pragma once
#include "Vector3.h"
#include <vector>

class Rail {
public:
    // レールの制御点を初期化する
    void Initialize();

    // 割合 tの位置における座標を取得する
    Vector3 GetPosition(float t) const;

    // 割合 t の位置における前方ベクトルを取得する
    Vector3 GetForward(float t) const;

    std::vector<Vector3>& GetControlPoints() { return controlPoints_; }

private:
    std::vector<Vector3> controlPoints_;
};