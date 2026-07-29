#pragma once
#include <Windows.h>
#include <Xinput.h>

class GamePad {
public:
	GamePad();
	~GamePad() = default;

	//更新処理
	void Update();

	// ボタンが押されているか
	bool GetButton(WORD button);

	// ボタンが押された瞬間か
	bool GetButtonTrigger(WORD button);

	// 左スティックの入力
	void GetLeftStick(float& outX, float& outY);

	// 右スティックの入力
	void GetRightStick(float& outX, float& outY);

	// 左トリガーの押し込み量
	float GetLeftTrigger();

	// 右トリガーの押し込み量
	float GetRightTrigger();

	void DrawImGui();

private:
	XINPUT_STATE state_;
	XINPUT_STATE prevState_;
	bool isConnected_ = false;
};