#include "TimedCall.h"

// コンストラクタ
TimedCall::TimedCall(std::function<void()> callback, uint32_t time)
	: callback_(callback), time_(time), isFinished_(false) {
}

// 更新処理

void TimedCall::Update() {
	if (isFinished_) {
		return;
	}

	// 残り時間を減らす
	time_--;

	if (time_ <= 0) {
		// 完了フラグを立てる
		isFinished_ = true;

		if (callback_) {
			callback_();
		}
	}

	}