#pragma once
#include <functional>
#include <cstdint>

class TimedCall {

public:
	// コンストラクタ
	TimedCall(std::function<void()> callback, uint32_t time);

	// 更新
	void Update();

	// 完了ならtrueを返す
	bool isFinished() const { return isFinished_; }

private:

	// コールバック
	std::function<void()> callback_;

	// 残り時間
	uint32_t time_;

	// 完了フラグ
	bool isFinished_ = false;
};
