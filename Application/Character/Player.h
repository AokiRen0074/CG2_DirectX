#pragma once
#include "Model.h"
#include "WorldTransform.h"
#include "Input.h"
#include "Vector3.h"
#include "PlayerBullet.h"
#include <list>
#include "Collider.h"
class Player: public Collider {
public:

	/*-------------------------
	プレイヤー系
	-------------------------*/
	// 初期化
	void Initialize(Object3d* model, uint32_t textureHandle);
	// 更新処理
	void Update();

	// 描画処理
	void Draw(const ViewProjection& viewProjection);

	// 弾のモデルのセット
//	void SetBulletModel(Object3d* bulletModel) { bulletModel_ = bulletModel; }

	// 旋回処理
	void Rotate();

	// 攻撃
	void Attack();



	void OnCollision() override;
	Vector3 GetWorldPosition() override;



	/*----------------------------
	めちゃ便利
	-----------------------------*/
	// 調整項目の運用
	void ApplyGlobalVariables();
	// 調整項目を登録
	static void RegisterGlobalVariables();



	// 弾リストの取得
	const std::list<PlayerBullet*>& GetBullets() const { return bullets_; }

	/*------------------
	デストラクタ
	----------------------------*/
	~Player();

private:

	WorldTransform worldTransform_;

	// モデル
	Object3d* model_ = nullptr;

	// テクスチャハンドル
	uint32_t textureHandle_ = 0u;

	// keyboard入力
	Input* input_ = nullptr;


	/*-----------------------
	プレイヤーについての変数
	-----------------------------*/
	// プレイヤーの速さ
	static inline float kCharacterSpeed = 0.2f;

	/*-------------------------
	弾
	-----------------------------*/
	PlayerBullet* bullet_ = nullptr;
	std::list<PlayerBullet*>bullets_;

};