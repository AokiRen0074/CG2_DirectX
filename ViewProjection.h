#pragma once
#include "Matrix4x4.h"
#include "Vector3.h"

struct ViewProjection {
	// カメラ座標と回転
	Vector3 translation_ = { 0.0f,0.0f,-50.0f };
	Vector3 rotation_ = { 0.0f,0.0f,0.0f };

	// ビュー行列とプロジェクション行列
	Matrix4x4 matView;
	Matrix4x4 matProjection;

	// 初期化と更新
	void Initialize();
	void UpdateMatrix();
};