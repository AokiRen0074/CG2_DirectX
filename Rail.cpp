#include "Rail.h"
#include "MathUtility.h"
#include <cmath>

void Rail::Initialize() {

	controlPoints_ = {
		{ 0.0f,  0.0f, -50.0f },
		{ 0.0f,  0.0f,   0.0f }, // [1] スタート地点
		{ 0.0f,  5.0f,  50.0f }, // [2] 上に登る
		{ 20.0f, 0.0f, 100.0f }, // [3] 右に大きくカーブ
		{-20.0f,-5.0f, 150.0f }, // [4] 左下へカーブ
		{ 0.0f,  0.0f, 200.0f }, // [5] ゴール地点
		{ 0.0f,  0.0f, 250.0f }  
	};
}

Vector3 Rail::GetPosition(float t) const {
	// 先ほど作成したCatmull-Rom補間関数を呼び出す
	return CatmullRomPosition(controlPoints_, t);
}

Vector3 Rail::GetForward(float t) const {
	// 現在の座標
	Vector3 pos = GetPosition(t);

	// 少し先の座標
	float nextT = std::fmin(t + 0.001f, 1.0f);
	Vector3 nextPos = GetPosition(nextT);

	// 差分ベクトルを求めて正規化
	Vector3 forward = nextPos - pos;
	return Normalize(forward);
}