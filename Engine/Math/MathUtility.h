#pragma once
#include "Vector3.h"
#include <vector>

// 4つの制御点から補間する関数
Vector3 CatmullRomInterpolation(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t);

// 制御点の配列と割合から、スプライン曲線上の座標を得る関数
Vector3 CatmullRomPosition(const std::vector<Vector3>& points, float t);