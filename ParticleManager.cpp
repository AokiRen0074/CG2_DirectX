#include "ParticleManager.h"
#include <random>

void ParticleManager::Initialize(NeonModel* model, NeonModel* starModel, uint32_t textureHandle) {
	particleModel_ = model;
	starModel_ = starModel; 
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

void ParticleManager::EmitStar(const Vector3& position, int count, const Vector3& color) {
	for (int i = 0; i < count; ++i) {
		float theta = (float)(rand() % 628) / 100.0f;
		Vector3 dir = { std::cos(theta), std::sin(theta), 0.0f };
		float speed = 2.0f + (float)(rand() % 50) / 10.0f; // 爆発的な初速
		float thickness = 0.05f + (float)(rand() % 30) / 1000.0f;

		NeonParticle* newParticle = new NeonParticle();
		newParticle->Initialize(starModel_, position, dir, speed, thickness, color);
		particles_.push_back(newParticle);
	}
}

void ParticleManager::Emit(const Vector3& position, int count, const Vector3& color) {

	// 四方八方に飛び散る火花
	for (int i = 0; i < count; ++i) {
		float theta = (float)(rand() % 628) / 100.0f; // 0 〜 2π

		Vector3 dir;
		dir.x = std::cos(theta);
		dir.y = std::sin(theta);
		dir.z = 0.0f; 

		// 初期スピード（ドカン！という勢い）
		float speed = 1.0f + (float)(rand() % 40) / 10.0f; // 1.0 〜 5.0のランダム


		float thickness = 0.02f + (float)(rand() % 20) / 1000.0f;


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