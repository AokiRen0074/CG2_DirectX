#include "MathUtility.h"
#include <cassert>
#include <cmath>

Vector3 CatmullRomInterpolation(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) {
	const float s = 0.5f;

	float t2 = t * t;
	float t3 = t2 * t;

	Vector3 e3 = -p0 + 3 * p1 - 3 * p2 + p3;
	Vector3 e2 = 2 * p0 - 5 * p1 + 4 * p2 - p3;
	Vector3 e1 = -p0 + p2;
	Vector3 e0 = 2 * p1;

	return s*(e3 * t3 + e2 * t2 + e1 * t + e0);
}

Vector3 CatmullRomPosition(const std::vector<Vector3>& points, float t) {
	assert(points.size() >= 4 && "制御点は4点以上必要です");

	// 区間数は制御点の数-1
	size_t division = points.size() - 1;
	// 1区間の長さ 
	float areaWidth = 1.0f / division;

	// 区間内の始点を0.0、終点を1.0としたときの現在位置
	float t_2 = std::fmod(t, areaWidth) * division;
	// 下限と上限の範囲に収める
	t_2 = std::fmax(0.0f, std::fmin(1.0f, t_2));

	// 区間番号
	size_t index = static_cast<size_t>(t / areaWidth);
	// 上限を超えないように収める
	if (index >= division) {
		index = division - 1;
		t_2 = 1.0f; // 
	}

	// 4点分のインデックス
	size_t index0 = (index == 0) ? 0 : index - 1;
	size_t index1 = index;
	size_t index2 = index + 1;
	size_t index3 = (index + 2 >= points.size()) ? points.size() - 1 : index + 2;

	// 4点の座標
	const Vector3& p0 = points[index0];
	const Vector3& p1 = points[index1];
	const Vector3& p2 = points[index2];
	const Vector3& p3 = points[index3];

	// 4点を指定して補間
	return CatmullRomInterpolation(p0, p1, p2, p3, t_2);
}