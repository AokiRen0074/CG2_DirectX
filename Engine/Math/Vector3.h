#pragma once

class Vector3 {

public:

	float x;
	float y;
	float z;

	// コンストラクタ
	Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
};

// 三次元ベクトルの加算
Vector3 Add(const Vector3& v1, const Vector3& v2);

// 三次元ベクトルの減算
Vector3 Subtract(const Vector3& v1, const Vector3& v2);

// 三次元ベクトルのスカラー倍
Vector3 Multiply(float scalar, const Vector3& v);

// 三次元ベクトルの内積
float Dot(const Vector3& v1, const Vector3& v2);

// 三次元ベクトルの長さ
float Length(const Vector3& v);

// 三次元ベクトルの正規化
Vector3 Normalize(const Vector3& v);


