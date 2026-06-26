#pragma once
#include "WorldTransform.h"
#include "Object3d.h"
#include "TextureManager.h"

class Enemy {
public:

	// 行動フェーズ
	enum class Phase {
		Approach,// 接近
		Leave,// 離脱
	};

	// 初期化
	void Initialize(Object3d* model, uint32_t textureHandle);

	// 更新処理
	void Update();

	// 描画処理
	void Draw(const ViewProjection& viewProjection);

	// 接近フェーズ
	void ApproachPhase();

	// 離脱フェーズ
	void LeavePhase();

private:

	WorldTransform worldTransform_;

	// モデル
	Object3d* model_ = nullptr;

	// テクスチャハンドル
	uint32_t textureHandle_ = 0u;

	// フェーズ
	Phase phase_ = Phase::Approach;


	// メンバ関数ポインタのテーブル
	static void(Enemy::* phaseTable[])();



	// 接近速度
	static inline Vector3 approachVelocity_ = { 0.0f, 0.0f, -0.1f };

	// 離脱速度
	static inline Vector3 leaveVelocity_ = { 0.0f,0.0f,0.1f };
};