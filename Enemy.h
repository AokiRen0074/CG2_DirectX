#pragma once
#include "WorldTransform.h"
#include "Object3d.h"
#include "TextureManager.h"
#include "EnemyBullet.h"
#include <list>
#include "TimedCall.h"
#include "Collider.h"
#include "BodyModel.h"
#include "NeonModel.h"

class BaseEnemyState;
class Player;

class Enemy: public Collider{
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

	void DrawNeon(const ViewProjection& viewProjection);

	void DrawImGui();

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



	// 弾の発射
	void Fire();

	//　弾を発射し、タイマーをリセットする
	void FireAndReset();

	// フェーズ移行時にタイマーを消す
	void ClearTimedCalls();

	void SetPlayer(Player* player) { player_ = player; }


	void OnCollision() override;
	Vector3 GetWorldPosition() override;

	// 弾リストの取得
		// 弾リストの取得
	const std::list<EnemyBullet*>& GetBullets() const { return bullets_; }

private:

	WorldTransform worldTransform_;

	// モデル
	Object3d* model_ = nullptr;

	// まわすやつ
	WorldTransform transformLines_;

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

	

	/*---------------
	ネオン
	--------------------*/
	BodyModel* modelBase_ = nullptr;     // 暗い実体
	NeonModel* modelLines_ = nullptr;    // 光るライン・コア
	NeonModel* modelTails_[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
	NeonModel* modelRing_ = nullptr;     // バリアリング

	// 質感パラメータ
	float bodyColor_[3] = { 0.2f, 0.0f, 0.3f }; // 暗い紫
	float neonColor_[3] = { 0.8f, 0.0f, 1.0f }; // 鮮やかな紫/ピンク

	// 尻尾
	float tailColor1_[3] = { 0.8f, 0.0f, 1.0f }; // 紫
	float tailColor2_[3] = { 0.0f, 1.0f, 0.8f }; // 水色
	float tailColor3_[3] = { 1.0f, 0.0f, 0.5f }; // ピンク
	float tailIntensity_ = 12.0f;
	float neonIntensity_ = 10.0f;

	uint32_t dummyTexture_ = 0;
	uint32_t tailTexture_ = 0;

	// アニメーション尻尾
	WorldTransform transformTails_[5];
	float time_ = 0.0f;

	/*---------------------------------------
	
	*/
	// 時限発動のリスト
	std::list<TimedCall*> timedCalls_;

};