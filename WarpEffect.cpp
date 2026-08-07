#include "WarpEffect.h"
#include <cmath>

void WarpEffect::Initialize(DirectXCommon* dxCommon) {
	// 🌟 用意した真っ白なテクスチャを読み込む（これで丸っこくならない！）
	textureHandle_ = TextureManager::Load("Resources/block.png");

	std::random_device seed_gen;
	randomEngine_.seed(seed_gen());

	for (int i = 0; i < kMaxLines; ++i) {
		// 🌟 線ごとに専用のモデルを生成する（DirectXの仕様上、こうしないと色が全部同じになる）
		models_[i] = new NeonModel();
		models_[i]->Initialize("Resources", "block.obj");

		lines_[i].transform.Initialize();
		ResetLine(i, true);
	}
}

void WarpEffect::ResetLine(int index, bool isInitialSpawn) {
	WarpLine& line = lines_[index];

	// ==========================================
	// 🌟 動画の色味を再現（青紫、ピンク、水色などが混ざるように）
	// ==========================================
	std::uniform_real_distribution<float> rDist(0.0f, 1.0f);
	std::uniform_real_distribution<float> bDist(0.8f, 1.0f);
	line.color[0] = rDist(randomEngine_); // 赤成分ランダム
	line.color[1] = 0.0f;                 // 緑は0（これでサイバーパンクな紫〜ピンク系になる）
	line.color[2] = bDist(randomEngine_); // 青成分は高め

	// 動画のようなハイスピード感を出すための速度設定
	std::uniform_real_distribution<float> speedDist(5.0f, 15.0f);
	line.baseSpeed = speedDist(randomEngine_);

	// 長さのランダム幅
	std::uniform_real_distribution<float> lengthDist(10.0f, 30.0f);
	line.baseLength = lengthDist(randomEngine_);

	// 画面中央を空けて、奥から手前へのトンネル状に配置
	std::uniform_real_distribution<float> angleDist(0.0f, 3.141592f * 2.0f);
	std::uniform_real_distribution<float> radiusDist(15.0f, 70.0f);

	float angle = angleDist(randomEngine_);
	float radius = radiusDist(randomEngine_);

	line.transform.translation_.x = std::cos(angle) * radius;
	line.transform.translation_.y = std::sin(angle) * radius;

	if (isInitialSpawn) {
		std::uniform_real_distribution<float> zDist(0.0f, 500.0f);
		line.distanceZ = zDist(randomEngine_);
	}
	else {
		std::uniform_real_distribution<float> zDist(500.0f, 600.0f);
		line.distanceZ = zDist(randomEngine_);
	}
}

void WarpEffect::Update(float intensity) {
	for (int i = 0; i < kMaxLines; ++i) {
		WarpLine& line = lines_[i];

		// 手前に向かって移動
		line.distanceZ -= line.baseSpeed * intensity;

		// ==========================================
		// 🌟 動画のような「細長い針」にするためのスケーリング魔法
		// ==========================================
		line.transform.scale_.x = 0.05f; // Xを極細に
		line.transform.scale_.y = 0.05f; // Yを極細に

		// Z(奥行き)は、移動速度と強度に合わせて「残像」のように超絶長く伸ばす！
		line.transform.scale_.z = (line.baseLength * intensity) + 30.0f;

		line.transform.translation_.z = line.distanceZ;

		// カメラを通り過ぎたら奥でリセット
		if (line.distanceZ < -30.0f) {
			ResetLine(i, false);
		}

		line.transform.matWorld_ = MakeAffineMatrix(line.transform.scale_, line.transform.rotation_, line.transform.translation_);
		line.transform.TransferMatrix();
	}
}

void WarpEffect::Draw(const ViewProjection& viewProjection) {
	for (int i = 0; i < kMaxLines; ++i) {
		// 🌟 色をバッチリ飛ばすために輝度を20.0fなどに設定
		models_[i]->SetNeonColor(20.0f, lines_[i].color[0], lines_[i].color[1], lines_[i].color[2]);
		models_[i]->Draw(lines_[i].transform, viewProjection, textureHandle_);
	}
}