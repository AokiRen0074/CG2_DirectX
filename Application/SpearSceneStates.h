#pragma once
#include "Scene/IScene.h"
#include <memory>

class GameScene;
class SceneManager;

// ゲーム本体を共有し、状態の切り替えでモデル・音源・演出を読み直さない
class PlayingSceneState final : public IScene {
public:
	PlayingSceneState(std::shared_ptr<GameScene> gameScene, SceneManager& manager);

	void Update() override;
	void Draw() override;

private:
	std::shared_ptr<GameScene> gameScene_;
	SceneManager& manager_;
};

class ResultSceneState final : public IScene {
public:
	ResultSceneState(std::shared_ptr<GameScene> gameScene, SceneManager& manager);

	void Update() override;
	void Draw() override;

private:
	std::shared_ptr<GameScene> gameScene_;
	SceneManager& manager_;
};