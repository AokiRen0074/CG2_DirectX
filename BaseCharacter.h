#pragma once
#include "Object3d.h"
#include "WorldTransform.h"
#include "ViewProjection.h"

class BaseCharacter {
protected:

    WorldTransform worldTransform_;
    Object3d* model_ = nullptr;
    uint32_t textureHandle_ = 0u;

public:
    // 仮想デストラクタ
    virtual ~BaseCharacter() = default;

    // 共通の描画処理
    virtual void Draw(const ViewProjection& viewProjection);
};