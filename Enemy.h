#pragma once
#include "WorldTransform.h"
#include "Object3d.h"
#include "TextureManager.h"

class BaseEnemyState;

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


	// シーンを切り替える関数
	void ChangeState(BaseEnemyState* newState);

	// 指定した移動量だけ座標を変更する　カプセル化用
	void Move(const Vector3& velocity);

	// 座標のゲッター
	Vector3 GetTranslation() const;

private:

	WorldTransform worldTransform_;

	// モデル
	Object3d* model_ = nullptr;

	// テクスチャハンドル
	uint32_t textureHandle_ = 0u;

	// 状態を管理するポインタ
	BaseEnemyState* state_ = nullptr;


};