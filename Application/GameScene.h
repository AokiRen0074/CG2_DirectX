#pragma once
#include <memory>
#include "3D/Object3d.h"
#include "2D/Sprite.h"
#include "3D/DebugCamera.h"
#include "Audio/Audio.h"
#include "ViewProjection.h"
#include "DirectXCommon.h"
#include "Bloom.h"
#include "Character/SpearPrototype.h"



class Skydome;
class DirectXCommon;


class GameScene {
public:
    void BeginStageEditorSmokeTest() { spearPrototype_.BeginStageEditorSmokeTest(); }
    void OpenStageEditor() { spearPrototype_.OpenStageEditor(); }
    void OpenUiEditor(const char *page = "title") { spearPrototype_.OpenUiEditor(page); }
    void SetUiEditorTestSavePath() { spearPrototype_.SetUiEditorTestSavePath(); }
    void SetPaused(bool paused) { spearPrototype_.SetPaused(paused); }
    void RetryFromPause() { spearPrototype_.RetryFromPause(); }
    bool IsPaused() const { return spearPrototype_.IsPaused(); }
    std::string PauseSimulationStamp() const { return spearPrototype_.PauseSimulationStamp(); }
    void RestartRun() { spearPrototype_.RestartRun(); }
    bool IsResultReady() const { return spearPrototype_.IsResultReady(); }


	void Initialize(DirectXCommon* dxCommon);
	void Update();
	void Draw();

	        void PreviewPierce() {
        spearPrototype_.PreviewPierce();
    }

    void SetFixedPreviewStep() { spearPrototype_.SetFixedPreviewStep(); }

    void PreviewJumpCount(int charges) { spearPrototype_.PreviewJumpCount(charges); }
    void AudioSmokeAction(int action) { spearPrototype_.AudioSmokeAction(action); }
    std::string AudioStatus() const { return spearPrototype_.AudioStatus(); }
    void PreviewFinish(int point) { spearPrototype_.PreviewFinish(point); }
    void RequestFinishPhase() { spearPrototype_.RequestFinishPhase(); }
    void PreviewAirRoute() { spearPrototype_.PreviewAirRoute(); }
    void PreviewAirChain(bool strike) { spearPrototype_.PreviewAirChain(strike); }
    void PreviewTutorialRefill() { spearPrototype_.PreviewTutorialRefill(); }
    void PreviewDodge() {
        spearPrototype_.PreviewDodge();
    }
    void PreviewBossAttack(int role) {
        spearPrototype_.PreviewBossAttack(role);
    }

    void PreviewBossDefeat() {
        spearPrototype_.PreviewBossDefeat();
    }
    void OpenBossPractice() {
		spearPrototype_.OpenBossPractice();
	}

	void OpenSpearEditor() {
		spearPrototype_.OpenEditor();
	}



	~GameScene();

private:

	// 槍の操作を試すステージ（既存のプレイヤーとは独立）。
	SpearPrototype spearPrototype_;

	DirectXCommon* dxCommon_ = nullptr; 
	std::unique_ptr<Bloom> bloom_;
    std::unique_ptr<Bloom> effectBloom_;
    EffectRenderer effectRenderer_;


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
	std::unique_ptr<DebugCamera> debugCamera_;
	bool isDebugCameraActive_ = false;

	// オブジェクト
	std::unique_ptr<Object3d> object3d_;
	std::unique_ptr<Sprite> sprite_;

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

