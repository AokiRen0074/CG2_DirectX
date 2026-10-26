#pragma once
#include "IScene.h"
#include <memory>

// 更新中のシーンをその場で破棄せず、更新が終わってから次の状態へ移る
class SceneManager {
public:
	~SceneManager();

	void ChangeScene(std::unique_ptr<IScene> scene);
	void Update();
	void Draw();
	void Clear() noexcept;

	const IScene* GetCurrentScene() const {
		return currentScene_.get();
	}

private:
	void ApplyPendingScene();

	std::unique_ptr<IScene> currentScene_;
	std::unique_ptr<IScene> pendingScene_;
};