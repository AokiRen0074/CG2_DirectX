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

    // ゲッター
    const ViewProjection& GetViewProjection() const { return viewProjection_; }
    const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }

    // エディター用のアクセッサ
    float GetT() const { return t_; }
    void SetT(float t) { t_ = t; }
    float* GetSpeedPtr() { return &speed_; }
    bool* GetIsPlayPtr() { return &isPlay_; }

private:
    Rail* rail_ = nullptr;
    float t_ = 0.0f;
    float speed_ = 0.001f;
    bool isPlay_ = true;


    Vector3 translate_;
    Vector3 rotate_;

    Matrix4x4 worldMatrix_;
    ViewProjection viewProjection_;
};