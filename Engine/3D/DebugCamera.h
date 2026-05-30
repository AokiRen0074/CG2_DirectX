#pragma once
#include "Matrix4x4.h"


class DebugCamera {
public:
	// 初期化
	void Initialize();

	// 更新
	void Update();

	// 行列を取得するためのゲッター
	const Matrix4x4& GetViewMatrix() const { return matView_; }
	const Matrix4x4& GetProjectionMatrix() const { return matProjection_; }

private:
	// 累積回転行列
	Matrix4x4 matRot_;

	// ローカル座標
	Vector3 translation_ = { 0.0f, 0.0f, -50.0f };

	// ビュー行列 カメラの視点
	Matrix4x4 matView_;

	// 射影行列 
	Matrix4x4 matProjection_;

	float rotX_ = 0.0f;
	float rotY_ = 0.0f;
};