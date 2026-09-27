#pragma once
#include "3D/Object3d.h"
#include "2D/Sprite.h"
#include "3D/DebugCamera.h"
#include "Audio/Audio.h"
#include "ViewProjection.h"
#include "DirectXCommon.h"
#include "Bloom.h"



class Skydome;
class DirectXCommon;


class GameScene {
public:


	void Initialize(DirectXCommon* dxCommon);
	void Update();
	void Draw();


	~GameScene();

private:

	DirectXCommon* dxCommon_ = nullptr; 
	Bloom* bloom_ = nullptr;


	//ModelData* modelData_ = nullptr;
	uint32_t textureHandle_ = 0u;


	/*-----------------------------
	天球
	----------------------------------*/
	//Object3d* skydomeModel_ = nullptr;
	//Skydome* skydome_ = nullptr;
	// 天球用のテクスチャハンドル
	//uint32_t skydomeTex_ = 0u;

	// デバッグカメラ
	DebugCamera* debugCamera_ = nullptr;
	bool isDebugCameraActive_ = false;

	// オブジェクト
	Object3d* object3d_ = nullptr;
	Sprite* sprite_ = nullptr;

	// ビュープロジェクション
	ViewProjection viewProjection_;

	WorldTransform worldTransform_;

	// 音声データ
	//SoundData soundData_;

	/*------------------
	音
	--------------------*/
	SoundData bgmSound_;
	SoundData warpSound_;
	IXAudio2SourceVoice* bgmVoice_ = nullptr;
	IXAudio2SourceVoice* warpVoice_ = nullptr;


};
