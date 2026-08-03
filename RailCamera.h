#pragma once
#include "Rail.h"
#include "ViewProjection.h"
#include "Matrix4x4.h"
#include "Vector3.h"

class RailCamera {
public:
    // 走る対象のレールを受け取って初期化
    void Initialize(Rail* rail);

    // 毎フレームの更新
    void Update();

    // ゲッター群
    const ViewProjection& GetViewProjection() const { return viewProjection_; }
    const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }

private:
    Rail* rail_ = nullptr;
    float t_ = 0.0f;
    float speed_ = 0.001f;

    Vector3 translate_;
    Vector3 rotate_;

    Matrix4x4 worldMatrix_;
    ViewProjection viewProjection_;
};