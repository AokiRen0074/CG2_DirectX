#include "SceneManager.h"
#include <stdexcept>

SceneManager::~SceneManager() {
	Clear();
}

void SceneManager::ChangeScene(std::unique_ptr<IScene> scene) {
	if (!scene) {
		throw std::invalid_argument("A scene must not be null");
	}

	pendingScene_ = std::move(scene);
}

void SceneManager::ApplyPendingScene() {
	if (!pendingScene_) {
		return;
	}

	// OnExitで次の変更が予約されても、今回の遷移先と混ざらないようにする
	auto nextScene = std::move(pendingScene_);

	if (currentScene_) {
		currentScene_->OnExit();
	}

	currentScene_ = std::move(nextScene);
	currentScene_->OnEnter();
}

void SceneManager::Update() {
	ApplyPendingScene();

	if (currentScene_) {
		currentScene_->Update();
	}

	// この時点では元のシーンのUpdateから戻っているため、安全に破棄できる
	ApplyPendingScene();
}

void SceneManager::Draw() {
	if (currentScene_) {
		currentScene_->Draw();
	}
}

void SceneManager::Clear() noexcept {
	pendingScene_.reset();

	if (currentScene_) {
		currentScene_->OnExit();
		currentScene_.reset();
	}
}