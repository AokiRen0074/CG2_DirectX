#include "WarpEffect.h"
#include <cmath>

void WarpEffect::Initialize(DirectXCommon* dxCommon) {
	// ⚠️ 超重要：ここは絶対に「フチのない完全な真っ白な画像」を指定してください！
	// もし ring.png などになっていると、動画前半のような点線になってしまいます。
	textureHandle_ = TextureManager::Load("Resources/white.png");

	std::random_device seed_gen;
	randomEngine_.seed(seed_gen());

	for (int i = 0; i < kColorPatterns; ++i) {
		models_[i] = new NeonModel();
		models_[i]->Initialize("Resources", "block.obj");
	}

	// ==========================================
	// 🌟 ネオンの魔法：コアを白くするために、0.0f ではなく 0.4f などを混ぜる！
	// ==========================================
	float intensity = 12.0f; // 輝度（ブルームの強さ）

	// ① マゼンタ (白コア + ピンクオーラ)
	models_[0]->SetNeonColor(intensity, 1.0f, 0.4f, 1.0f);

	// ② シアン (白コア + 水色オーラ)
	models_[1]->SetNeonColor(intensity, 0.4f, 1.0f, 1.0f);

	// ③ パープル (白コア + 青紫オーラ)
	models_[2]->SetNeonColor(intensity, 0.6f, 0.4f, 1.0f);

	for (int i = 0; i < kMaxLines; ++i) {
		lines_[i].transform.Initialize();
		ResetLine(i, true);
	}
}

void WarpEffect::ResetLine(int index, bool isInitialSpawn) {
	WarpLine& line = lines_[index];

	std::uniform_int_distribution<int> colorDist(0, 2);
	line.colorIndex = colorDist(randomEngine_);

	// 速度の幅を広げる（速い線と遅い線が入り乱れることで奥行きが出る）
	std::uniform_real_distribution<float> speedDist(8.0f, 25.0f);
	line.baseSpeed = speedDist(randomEngine_);

	// 線のベースの長さを大幅に長くする
	std::uniform_real_distribution<float> lengthDist(30.0f, 80.0f);
	line.baseLength = lengthDist(randomEngine_);

	// 発生範囲を広げて、画面全体を包み込むようにする
	std::uniform_real_distribution<float> angleDist(0.0f, 3.141592f * 2.0f);
	std::uniform_real_distribution<float> radiusDist(20.0f, 120.0f);

	float angle = angleDist(randomEngine_);
	float radius = radiusDist(randomEngine_);

	line.transform.translation_.x = std::cos(angle) * radius;
	line.transform.translation_.y = std::sin(angle) * radius;

	if (isInitialSpawn) {
		std::uniform_real_distribution<float> zDist(0.0f, 800.0f);
		line.distanceZ = zDist(randomEngine_);
	}
	else {
		// 再スタート位置をさらに奥へ
		std::uniform_real_distribution<float> zDist(800.0f, 1000.0f);
		line.distanceZ = zDist(randomEngine_);
	}
}

void WarpEffect::Update(float intensity) {
	currentIntensity_ += (intensity - currentIntensity_) * 0.015f;


	if (intensity == 0.0f && currentIntensity_ < 1.0f) {
		currentIntensity_ = 0.0f;
	}

	float ratio = currentIntensity_ / 15.0f;
	if (ratio < 0.0f) ratio = 0.0f;

	for (int i = 0; i < kMaxLines; ++i) {
		WarpLine& line = lines_[i];

		line.distanceZ -= line.baseSpeed * currentIntensity_;

		line.transform.scale_.x = 0.08f * ratio;
		line.transform.scale_.y = 0.08f * ratio;

		// ==========================================
		// 🌟 修正：長さ（Z）全体にも ratio を掛ける。
		// これにより、消える瞬間は線が「短く」なりながらスッと消滅する！
		// ==========================================
		line.transform.scale_.z = ((line.baseLength * currentIntensity_) + 50.0f) * ratio;

		line.transform.translation_.z = line.distanceZ;

		if (line.distanceZ < -50.0f) {
			ResetLine(i, false);
		}

		line.transform.matWorld_ = MakeAffineMatrix(line.transform.scale_, line.transform.rotation_, line.transform.translation_);
		line.transform.TransferMatrix();
	}
}

void WarpEffect::Draw(const ViewProjection& viewProjection) {
	// 🌟 修正：0.1f 未満ではなく、完全に 0.0f 以下の時だけスキップする
	if (currentIntensity_ <= 0.0f) return;

	for (int i = 0; i < kMaxLines; ++i) {
		int cIdx = lines_[i].colorIndex;
		models_[cIdx]->Draw(lines_[i].transform, viewProjection, textureHandle_);
	}
}