#pragma once
#include "Model.h"
#include "WorldTransform.h"
#include "Input.h"
#include "Vector3.h"
#include "PlayerBullet.h"
#include <list>
#include "Collider.h"
#include "NeonModel.h"
#include "BodyModel.h"

class BaseEnemy;

class Player: public Collider {
public:

	/*-------------------------
	プレイヤー系
	-------------------------*/
	// 初期化
	void Initialize();
	// 更新処理
	void Update(const Matrix4x4& parentMatrix);

	// 描画処理
	void Draw(const ViewProjection& viewProjection);

	// ネオンのもの
	void DrawNeon(const ViewProjection& viewProjection);

	// 弾のモデルのセット
//	void SetBulletModel(Object3d* bulletModel) { bulletModel_ = bulletModel; }

	// 旋回処理
	void Rotate();

	// 攻撃
	void Attack();

	// ImGui描画関数
	void DrawImGui();

	void SetEnemies(const std::list<BaseEnemy*>* enemies) { enemies_ = enemies; }


	void OnCollision() override;
	Vector3 GetWorldPosition() override;



	/*----------------------------
	めちゃ便利
	-----------------------------*/
	// 調整項目の運用
	void ApplyGlobalVariables();
	// 調整項目を登録
	static void RegisterGlobalVariables();

	// 点光源設置
	void SetPointLight(const Vector3& pos, const Vector3& color, float intensity, float radius, const Vector3& cameraPos);

	// 弾リストの取得
	const std::list<PlayerBullet*>& GetBullets() const { return bullets_; }

	const Vector3& GetTranslation() const { return worldTransform_.translation_; }

	const Vector3& GetRotation() const { return worldTransform_.rotation_; }

	/*------------------
	デストラクタ
	----------------------------*/
	~Player();

private:

	const std::list<BaseEnemy*>* enemies_ = nullptr;

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

	// ネオン調整用
	float neonColor_[3] = { 1.0f, 0.2f, 1.0f }; // RGB
	float neonIntensity_ = 8.0f;                // 光の強さ

	//  暗いボディ調整用の変数（初期値は黒紫）
	float bodyColor_[3] = { 0.1f, 0.05f, 0.15f }; // RGB

	// --- 暗いパーツ---
	BodyModel* modelCore_ = nullptr;
	BodyModel* modelOuterRing_ = nullptr;
	BodyModel* modelWingBase_ = nullptr;

	// --- 光るパーツ）---
	NeonModel* modelInnerRing_ = nullptr; //  中のリング 
	NeonModel* modelWingNeon_ = nullptr; //  羽の光る部分 



	WorldTransform transformRot_;  // 回るパーツ用
	WorldTransform transformStat_; // 回らないパーツ用



	float coreSpinAngle_ = 0.0f;   // 回転角度タイマー

	// 嘘
	uint32_t dummyTexture_ = 0u;

	/*-------------------------
	弾
	-----------------------------*/
	PlayerBullet* bullet_ = nullptr;
	std::list<PlayerBullet*>bullets_;

	NeonModel* bulletModel_ = nullptr;

};