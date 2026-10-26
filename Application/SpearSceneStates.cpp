#include "SpearSceneStates.h"
#include "GameScene.h"
#include "Scene/SceneManager.h"
#include <stdexcept>

PlayingSceneState::PlayingSceneState(
	std::shared_ptr<GameScene> gameScene, SceneManager& manager)
	: gameScene_(std::move(gameScene)), manager_(manager) {
	if (!gameScene_) {
		throw std::invalid_argument("Playing scene requires game data");
	}
}

void PlayingSceneState::Update() {
	gameScene_->Update();

	if (gameScene_->IsResultReady()) {
		manager_.ChangeScene(std::make_unique<ResultSceneState>(gameScene_, manager_));
	}
}

void PlayingSceneState::Draw() {
	gameScene_->Draw();
}

ResultSceneState::ResultSceneState(
	std::shared_ptr<GameScene> gameScene, SceneManager& manager)
	: gameScene_(std::move(gameScene)), manager_(manager) {
	if (!gameScene_) {
		throw std::invalid_argument("Result scene requires game data");
	}
}

void ResultSceneState::Update() {
	// リプレイの入力と暗転は既存のゲーム処理を使い、演出の連続性を保つ
	gameScene_->Update();

	if (!gameScene_->IsResultReady()) {
		manager_.ChangeScene(std::make_unique<PlayingSceneState>(gameScene_, manager_));
	}
}

void ResultSceneState::Draw() {
	gameScene_->Draw();
}