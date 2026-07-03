#include "Vector3.h"
#include <cmath>
#include <algorithm>


static const int kColumnWidth = 60;
static const int kRowHeight = 20;

// 三次元ベクトルの加算
Vector3 Add(const Vector3& v1, const Vector3& v2) {
	Vector3 result;
	result.x = v1.x + v2.x;
	result.y = v1.y + v2.y;
	result.z = v1.z + v2.z;
	return result;
}

// 三次元ベクトルの減算
Vector3 Subtract(const Vector3& v1, const Vector3& v2) {
	Vector3 result;
	result.x = v1.x - v2.x;
	result.y = v1.y - v2.y;
	result.z = v1.z - v2.z;
	return result;
}

// 三次元ベクトルのスカラー倍
Vector3 Multiply(float scalar, const Vector3& v) {
	Vector3 result;
	result.x = scalar * v.x;
	result.y = scalar * v.y;
	result.z = scalar * v.z;
	return result;
}

// 三次元ベクトルの内積
float Dot(const Vector3& v1, const Vector3& v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

// 三次元ベクトルの長さ
float Length(const Vector3& v) {
	return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

// 三次元ベクトルの正規化
Vector3 Normalize(const Vector3& v) {
	Vector3 result;
	float length = Length(v);
	if (length != 0.0f) {
		result.x = v.x / length;
		result.y = v.y / length;
		result.z = v.z / length;
	}
	return result;
}

Vector3 Slerp(const Vector3& v1, const Vector3& v2, float t) {
	float dot = Dot(v1, v2);
	dot = std::clamp(dot, -1.0f, 1.0f); // 誤差吸収

	float theta = std::acos(dot); // 2つのベクトルのなす角

	// 角度がほぼ0の場合は、通常の線形補間で返す
	if (std::abs(theta) < 0.001f) {
		return {
			v1.x * (1.0f - t) + v2.x * t,
			v1.y * (1.0f - t) + v2.y * t,
			v1.z * (1.0f - t) + v2.z * t
		};
	}

	float sinTheta = std::sin(theta);
	float s1 = std::sin((1.0f - t) * theta) / sinTheta;
	float s2 = std::sin(t * theta) / sinTheta;

	return {
		v1.x * s1 + v2.x * s2,
		v1.y * s1 + v2.y * s2,
		v1.z * s1 + v2.z * s2
	};
}



