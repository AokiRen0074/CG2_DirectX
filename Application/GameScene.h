#pragma once
#include "3D/Object3d.h"
#include "2D/Sprite.h"
#include "3D/DebugCamera.h"
#include "Audio/Audio.h"
#include "ViewProjection.h"
#include "Application/Character/Player.h"
#include "BaseEnemy.h"
#include "NeonSign.h"
#include "DirectXCommon.h"
#include "Bloom.h"
#include "NeonText.h"
#include "NeonModel.h"
#include "NeonObj.h"
#include "ProceduralNeon.h"
#include "EditorPanel.h"
#include "FlickerTimer.h"
#include "Rail.h"
#include "RailCamera.h"
#include "RailEditor.h"
#include "WarpEffect.h"
#include "ParticleManager.h"
#include <list>

class Skydome;

class EnemyBullet;

class Collider;

class CollisionManager;

class DirectXCommon;

class WaveManager;

class GameScene {
public:

	void Initialize(DirectXCommon* dxCommon);
	void Update();
	void Draw();

	void AddEnemyBullet(EnemyBullet* enemyBullet);

	// 衝突マネージャーのポインタ
	CollisionManager* collisionManager_ = nullptr;

	~GameScene();

private:

	/*--------------------
	自キャラ
	------------------------*/
	Player* player_ = nullptr;

	//ModelData* modelData_ = nullptr;
	uint32_t textureHandle_ = 0u;

	/*---------------------
	敵キャラ
	-----------------------------*/
	std::list<BaseEnemy*> enemies_;
	std::list<EnemyBullet*> enemyBullets_;
	Object3d* enemyObject_ = nullptr;
	uint32_t enemyTex_ = 0u;

	/*-----------------------------
	天球
	----------------------------------*/
	Object3d* skydomeModel_ = nullptr;
	Skydome* skydome_ = nullptr;
	// 天球用のテクスチャハンドル
	uint32_t skydomeTex_ = 0u;

	// ゲームで使うカメラ
	DebugCamera* debugCamera_ = nullptr;
	bool isDebugCameraActive_ = false;

	// ゲームで使うオブジェクト達
	Object3d* object3d_ = nullptr;
	Sprite* sprite_ = nullptr;

	// ビュープロジェクション
	ViewProjection viewProjection_;

	// 音声データ
	SoundData soundData_;

	/*-------------------------
	ネオン
	---------------------------*/
	//Object3d* bulletModel_ = nullptr;
	NeonSign* neonSign_ = nullptr;
	float globalTubeLength_ = 2.0f;
	NeonText* neonText_ = nullptr;
	DirectXCommon* dxCommon_ = nullptr;
	Bloom* bloom_ = nullptr;
	NeonText* neonText_Open_ = nullptr;   // 文字用
	NeonText* neonText_Border_ = nullptr;

	NeonObj* myNeonBar_ = nullptr;
	ProceduralNeon* procNeon_ = nullptr;

	NeonModel* neonModel_ = nullptr;

	NeonModel* enemyBulletModel_ = nullptr;

	float neonRadius_ = 0.03f;
	float neonSoftness_ = 15.0f;
	float neonIntensity_ = 8.0f;
	float neonColor_[3] = { 0.0f, 0.8f, 1.0f };
	float neonLengthOffset_ = -0.2f;

	// チカチカを管理するクラス
	FlickerTimer neonTextFlicker_;

	/*------------------------
	エディター
	------------------------*/
	RailEditor* railEditor_ = nullptr;

	/*--------------------
	地面
	---------------------------*/
	Object3d* groundModel_ = nullptr;
	WorldTransform groundTransform_;
	uint32_t groundTex_ = 0u;

	/*-----------------------
	レールカメラ
	-------------------------*/
	Rail* rail_ = nullptr;
	RailCamera* railCamera_ = nullptr;

	/*-------------------------
	ウェーブ,敵スポーン管理
	----------------------*/


	WaveManager* waveManager_ = nullptr;

	// ワープエフェクト
	WarpEffect* warpEffect_ = nullptr;

	// カメラの傾き量
	float cameraRoll_ = 0.0f;

	/*------------------------------
	パーティクル
	------------------------------*/
	ParticleManager* particleManager_ = nullptr;
	NeonModel* particleModel_ = nullptr;
};
