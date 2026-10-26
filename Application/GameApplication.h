#pragma once
#include "Base/DirectXCommon.h"
#include "GameScene.h"
#include "Scene/SceneManager.h"
#include <memory>

// 起動・メインループ・終了をまとめ、main.cppには起動処理だけを残す
class GameApplication {
public:
	GameApplication() = default;
	~GameApplication();

	int Run(const char* commandLine);

	GameApplication(const GameApplication&) = delete;
	GameApplication& operator=(const GameApplication&) = delete;

private:
	int RunInternal(const char* commandLine);
	void Finalize() noexcept;

	std::unique_ptr<DirectXCommon> dxCommon_;
	std::shared_ptr<GameScene> gameScene_;
	SceneManager sceneManager_;
	bool comInitialized_ = false;
	bool loggerInitialized_ = false;
	bool windowInitialized_ = false;
};