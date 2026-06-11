#include "ViewProjection.h"

void ViewProjection::Initialize() {
	matView = MakeIdentity4x4();

	matProjection = MakePerspectiveFovMatrix(0.45f, 1280.0f / 720.0f, 0.1, 100.0f);
}

void ViewProjection::UpdateMatrix() {
	// 回転行列の作成
	Matrix4x4 rotX = MakeRotateXMatrix(rotation_.x);
	Matrix4x4 rotY = MakeRotateYMatrix(rotation_.y);
	Matrix4x4 rotZ = MakeRotateZMatrix(rotation_.z);
	Matrix4x4 matRot = Multiply(rotX, Multiply(rotY, rotZ);

	// 平行移動行列の作成
	Matrix4x4 matTrans = MakeTranslateMatrix(translation_);

	// カメラ自身のワールド行列
	Matrix4x4 matCameraWorld = Multiply(matRot, matTrans);

	// ビュー行列
	matView = Inverse(matCameraWorld);
}