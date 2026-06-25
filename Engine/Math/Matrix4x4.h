#pragma once
#include "Vector3.h"

class Matrix4x4 {
public:
	float m[4][4];

	// コンストラクタ
	Matrix4x4() {
		for (int i = 0; i < 4; ++i) {
			for (int j = 0; j < 4; ++j) {
				m[i][j] = 0.0f;
			}
		}
	}

	Matrix4x4(float xx, float xy, float xz, float xw,
		float yx, float yy, float yz, float yw,
		float zx, float zy, float zz, float zw,
		float wx, float wy, float wz, float ww) {
		m[0][0] = xx; m[0][1] = xy; m[0][2] = xz; m[0][3] = xw;
		m[1][0] = yx; m[1][1] = yy; m[1][2] = yz; m[1][3] = yw;
		m[2][0] = zx; m[2][1] = zy; m[2][2] = zz; m[2][3] = zw;
		m[3][0] = wx; m[3][1] = wy; m[3][2] = wz; m[3][3] = ww;
	}
};

// 4x4行列の加法
Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);

// 4x4行列の減法
Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);

// 4x4行列の積
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

// 4x4行列の転置行列
Matrix4x4 Transpose(const Matrix4x4& m1);

// 4x4行れ越の単位行列の作成
Matrix4x4 MakeIdentity4x4();

// 4x4行列の逆行列
Matrix4x4 Inverse(const Matrix4x4& m);

// 4x4行列の平行移動行列
Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

// 4x4行列の拡大縮小行列
Matrix4x4 MakeScaleMatrix(const Vector3& scale);

// 4x4行列の座標変換
Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);

// 4x4　X軸回転行列
Matrix4x4 MakeRotateXMatrix(float radian);

// 4x4　Y軸回転行列
Matrix4x4 MakeRotateYMatrix(float radian);

// 4x4　Z軸回転行列
Matrix4x4 MakeRotateZMatrix(float radian);

// 4x4　三次元アフィン変換行列 
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);


// ここから
// 4x4 投資投影行列
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

// 4x4 正射影行列
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

// 4x4 ビューポート行列
Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);

// クロス積
Vector3 Cross(const Vector3& v1, const Vector3& v2);

// ベクトル変換
Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m);

