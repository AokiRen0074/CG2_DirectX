#include "ParticleManager.h"
#include <random>

void ParticleManager::Initialize(NeonModel* model, uint32_t textureHandle) {
	particleModel_ = model;
	textureHandle_ = textureHandle;
}

void ParticleManager::Update() {
	// 全更新
	for (NeonParticle* particle : particles_) {
		particle->Update();
	}

	// 寿命が尽きた破片をリストから消してメモリ解放
	particles_.remove_if([](NeonParticle* particle) {
		if (particle->IsDead()) {
			delete particle;
			return true;
		}
		return false;
		});
}

void ParticleManager::Draw(const ViewProjection& viewProjection) {
	for (NeonParticle* particle : particles_) {
		particle->Draw(viewProjection, textureHandle_);
	}
}

void ParticleManager::Emit(const Vector3& position, int count, const Vector3& color) {

	// 四方八方に飛び散る火花
	for (int i = 0; i < count; ++i) {
		float theta = (float)(rand() % 628) / 100.0f; // 0 〜 2π

		Vector3 dir;
		dir.x = std::cos(theta);
		dir.y = std::sin(theta);
		dir.z = 0.0f; // 🌟 完全に画面(XY平面)と平行に広がるようにする

		// 初期スピード（ドカン！という勢い）
		float speed = 1.0f + (float)(rand() % 40) / 10.0f; // 1.0 〜 5.0のランダム

		// 🌟 限界まで細くする！（0.02f〜0.04f）これでもう四角いブロックには見えません
		float thickness = 0.02f + (float)(rand() % 20) / 1000.0f;

		// 🌟 色は引数を無視して、Wavecade特有の「シアン」か「ピンク」の2択に強制上書き！
		Vector3 particleColor;
		if (rand() % 2 == 0) {
			particleColor = { 0.0f, 0.8f, 1.0f }; // シアン
		}
		else {
			particleColor = { 1.0f, 0.0f, 0.8f }; // ピンク
		}

		NeonParticle* newParticle = new NeonParticle();
		newParticle->Initialize(particleModel_, position, dir, speed, thickness, particleColor);
		particles_.push_back(newParticle);
	}
}