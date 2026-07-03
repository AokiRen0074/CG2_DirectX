#pragma once
#include "WorldTransform.h"
#include "Object3d.h"
#include "TextureManager.h"
#include "EnemyBullet.h"
#include <list>
#include "TimedCall.h"

class BaseEnemyState;
class Player;

class Enemy {
public:

	// 行動フェーズ
	enum class Phase {
		Approach,// 接近
		Leave,// 離脱
	};

	// 発射間隔
	static const int kFireInterval = 60;

	// 初期化
	void Initialize(Object3d* model, uint32_t textureHandle);

	// 更新処理
	void Update();

	// 描画処理
	void Draw(const ViewProjection& viewProjection);

	// デストラクタ
	~Enemy();

	// 接近フェーズ初期化
	void ApproachPhaseInitialize();

	// タイマー更新
	void UpdateFireTimer();

	// シーンを切り替える関数
	void ChangeState(BaseEnemyState* newState);

	// 指定した移動量だけ座標を変更する　カプセル化用
	void Move(const Vector3& velocity);

	// 座標のゲッター
	Vector3 GetTranslation() const;

	// ワールド座標を取得
	Vector3 GetWorldPosition();

	// 弾の発射
	void Fire();

	//　弾を発射し、タイマーをリセットする
	void FireAndReset();

	// フェーズ移行時にタイマーを消す
	void ClearTimedCalls();

	void SetPlayer(Player* player) { player_ = player; }

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

	/*----------------
	自キャラ
	---------------------------*/
	Player* player_ = nullptr;

	


	/*---------------------------------------
	
	*/
	// 時限発動のリスト
	std::list<TimedCall*> timedCalls_;

};