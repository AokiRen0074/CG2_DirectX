#pragma once
#include "WorldTransform.h"
#include "Object3d.h"
#include "TextureManager.h"
#include "EnemyBullet.h"
#include <list>

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

	// デストラクタ
	~Enemy();

	// シーンを切り替える関数
	void ChangeState(BaseEnemyState* newState);

	// 指定した移動量だけ座標を変更する　カプセル化用
	void Move(const Vector3& velocity);

	// 座標のゲッター
	Vector3 GetTranslation() const;

	// 弾の発射
	void Fire();

private:

	WorldTransform worldTransform_;

	// モデル
	Object3d* model_ = nullptr;

	// テクスチャハンドル
	uint32_t textureHandle_ = 0u;

	// 状態を管理するポインタ
	BaseEnemyState* state_ = nullptr;

	/*---------------------------------------
	弾
	-----------------------------------*/
	EnemyBullet* bullet_ = nullptr;
	std::list<EnemyBullet*> bullets_;
	Enemy* enemy_ = nullptr;

	bool isFired_ = false;

};