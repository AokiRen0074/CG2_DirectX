#pragma once
#include "3D/Object3d.h"
#include "2D/Sprite.h"
#include "3D/DebugCamera.h"
#include "Audio/Audio.h"
#include "ViewProjection.h"
#include "Application/Character/Player.h"
#include "Enemy.h"

class Skydome;

class EnemyBullet;

class Collider;

class CollisionManager;

class DirectXCommon;

class GameScene {
public:

	void Initialize(DirectXCommon* dxCommon);
	void Update();
	void Draw();

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
	Enemy* enemy_ = nullptr;
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

	//Object3d* bulletModel_ = nullptr;
};
