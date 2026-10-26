#pragma once

// シーンごとの振る舞いを共通のインターフェースで切り替える
class IScene {
public:
	virtual ~IScene() = default;

	virtual void OnEnter() {
	}

	virtual void OnExit() noexcept {
	}

	virtual void Update() = 0;
	virtual void Draw() = 0;
};