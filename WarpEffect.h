#pragma once
#include "WorldTransform.h"
#include "ViewProjection.h"
#include "NeonModel.h"
#include "TextureManager.h"
#include <random>

class WarpEffect {
public:
	// 線の最大数
	static const int kMaxLines = 80;

	void Initialize(DirectXCommon* dxCommon);

	// 引数 演出の強さ
	void Update(float intensity);

	void Draw(const ViewProjection& viewProjection);

private:
	// 1本1本の線のデータ
	struct WarpLine {
		WorldTransform transform;
		float baseSpeed;    // 基本の移動速度
		float baseLength;   // 基本の長さ
		float color[3];     // ネオンカラー
		float distanceZ;    // 現在のZ座標
	};

	WarpLine lines_[kMaxLines];

	NeonModel* models_[kMaxLines] = { nullptr };

	NeonModel* model_ = nullptr;
	uint32_t textureHandle_ = 0u;

	// 乱数生成器
	std::mt19937 randomEngine_;

	// 線を遠くで再生成する関数
	void ResetLine(int index, bool isInitialSpawn);
};