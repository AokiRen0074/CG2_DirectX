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
class GameScene;

class BaseEnemy: public Collider{
public:

	// 行動フェーズ
	enum class Phase {
		Approach,// 接近
		Leave,// 離脱
	};

	// 発射間隔
	static const int kFireInterval = 60;

	// 初期化
	virtual void Initialize(Player* player);
	static void StaticInitialize();

	// 更新処理
	virtual void Update();

	// 描画処理
	virtual void Draw(const ViewProjection& viewProjection);

	virtual void DrawNeon(const ViewProjection& viewProjection);

	virtual void DrawImGui();

	void SetGameScene(GameScene* gameScene) { gameScene_ = gameScene; }

	// 死活判定用のゲッター
	bool IsDead() const { return isDead_; }

	void SetPosition(const Vector3& pos) {
		worldTransform_.translation_ = pos;

		worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
		worldTransform_.TransferMatrix();
	}

	// デストラクタ
	~BaseEnemy();

	// 接近フェーズ初期化
	void ApproachPhaseInitialize();

	// タイマー更新
	void UpdateFireTimer();

	// シーンを切り替える関数
	void ChangeState(BaseEnemyState* newState);

	// 指定した移動量だけ座標を変更する
	void Move(const Vector3& velocity);

	// 座標のゲッター
	Vector3 GetTranslation() const;



	// 弾の発射
	virtual void Fire();

	//　弾を発射し、タイマーをリセットする
	void FireAndReset();

	// フェーズ移行時にタイマーを消す
	void ClearTimedCalls();

	void SetPlayer(Player* player) { player_ = player; }


	void OnCollision() override;
	Vector3 GetWorldPosition() override;

	void SetSpawnIndex(int index) { spawnIndex_ = index; }
	int GetSpawnIndex() const { return spawnIndex_; }

protected:

	int spawnIndex_ = -1;

	GameScene* gameScene_ = nullptr;

	bool isDead_ = false;

	WorldTransform worldTransform_;

	// モデル
	NeonModel* model_ = nullptr;

	// まわすやつ
	WorldTransform transformLines_;

	// テクスチャハンドル
	uint32_t textureHandle_ = 0u;

	// 状態を管理するポインタ
	BaseEnemyState* state_ = nullptr;




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

	NeonModel* bulletModel_ = nullptr;

	// アニメーション尻尾
	WorldTransform transformTails_[5];
	float time_ = 0.0f;

	/*---------------------------------------
	
	*/
	// 時限発動のリスト
	std::list<TimedCall*> timedCalls_;

};