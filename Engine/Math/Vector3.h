#pragma once

class Vector3 {

public:

	float x;
	float y;
	float z;

	// コンストラクタ
	Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
	Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

	Vector3 operator+(const Vector3& obj) const {
		return Vector3(x + obj.x, y + obj.y, z + obj.z);
	}

	// ベクトル同士の減算
	Vector3 operator-(const Vector3& obj) const {
		return Vector3(x - obj.x, y - obj.y, z - obj.z);
	}

	// 単項のマイナス
	Vector3 operator-() const {
		return Vector3(-x, -y, -z);
	}

	// ベクトルのスカラー倍
	Vector3 operator*(float scalar) const {
		return Vector3(x * scalar, y * scalar, z * scalar);
	}
};

inline Vector3 operator*(float scalar, const Vector3& v) {
	return Vector3(v.x * scalar, v.y * scalar, v.z * scalar);
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

// Slerp
Vector3 Slerp(const Vector3& v1, const Vector3& v2, float t);


