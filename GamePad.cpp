#include "GamePad.h"
#include <cmath>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

#pragma comment(lib, "xinput.lib")

GamePad::GamePad() {
	// メモリをゼロクリアして安全に初期化
	ZeroMemory(&state_, sizeof(XINPUT_STATE));
	ZeroMemory(&prevState_, sizeof(XINPUT_STATE));
}

void GamePad::Update() {

	prevState_ = state_;
	ZeroMemory(&state_, sizeof(XINPUT_STATE));

	// コントローラーの最新状態を取得
	DWORD result = XInputGetState(0, &state_);
	isConnected_ = (result == ERROR_SUCCESS);
}

// ------------------------------------------
// ボタン入力
// ------------------------------------------
bool GamePad::GetButton(WORD button) {
	if (!isConnected_) return false;
	return (state_.Gamepad.wButtons & button) != 0;
}

bool GamePad::GetButtonTrigger(WORD button) {
	if (!isConnected_) return false;
	bool isCurrentDown = (state_.Gamepad.wButtons & button) != 0;
	bool isPrevDown = (prevState_.Gamepad.wButtons & button) != 0;
	return isCurrentDown && !isPrevDown;
}

// ------------------------------------------
// スティック入力
// ------------------------------------------
void GamePad::GetLeftStick(float& outX, float& outY) {
	outX = 0.0f;
	outY = 0.0f;
	if (!isConnected_) return;

	float stickX = state_.Gamepad.sThumbLX;
	float stickY = state_.Gamepad.sThumbLY;

	// 左スティック用のデッドゾーン処理
	if (std::abs(stickX) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ||
		std::abs(stickY) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
		outX = stickX / 32767.0f;
		outY = stickY / 32767.0f;
	}
}

void GamePad::GetRightStick(float& outX, float& outY) {
	outX = 0.0f;
	outY = 0.0f;
	if (!isConnected_) return;

	float stickX = state_.Gamepad.sThumbRX;
	float stickY = state_.Gamepad.sThumbRY;

	// ✨ 右スティック用のデッドゾーン処理
	if (std::abs(stickX) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ||
		std::abs(stickY) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE) {
		outX = stickX / 32767.0f;
		outY = stickY / 32767.0f;
	}
}

// ------------------------------------------
// トリガー入力
// ------------------------------------------
float GamePad::GetLeftTrigger() {
	if (!isConnected_) return 0.0f;
	BYTE trigger = state_.Gamepad.bLeftTrigger;

	if (trigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) {
		return trigger / 255.0f;
	}
	return 0.0f;
}

float GamePad::GetRightTrigger() {
	if (!isConnected_) return 0.0f;
	BYTE trigger = state_.Gamepad.bRightTrigger;

	if (trigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) {
		return trigger / 255.0f;
	}
	return 0.0f;
}

void GamePad::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("GamePad Tester");

	// コントローラーが繋がっていない場合の警告
	if (!isConnected_) {
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "GamePad is NOT connected!");
		ImGui::End();
		return;
	}

	// スティックの状態
	float lX, lY, rX, rY;
	GetLeftStick(lX, lY);
	GetRightStick(rX, rY);
	ImGui::Text("Left Stick  : X = % .2f, Y = % .2f", lX, lY);
	ImGui::Text("Right Stick : X = % .2f, Y = % .2f", rX, rY);

	ImGui::Separator();

	// トリガーの状態
	float lt = GetLeftTrigger();
	float rt = GetRightTrigger();
	ImGui::Text("LT (L2)"); ImGui::SameLine(70); ImGui::ProgressBar(lt, ImVec2(150, 0));
	ImGui::Text("RT (R2)"); ImGui::SameLine(70); ImGui::ProgressBar(rt, ImVec2(150, 0));

	ImGui::Separator();

	// ボタンの状態
	ImGui::Text("Buttons  :");
	ImGui::SameLine();

	if (GetButton(XINPUT_GAMEPAD_A)) ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "[ A ]");
	else ImGui::TextDisabled("[ A ]");
	ImGui::SameLine();

	if (GetButton(XINPUT_GAMEPAD_B)) ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "[ B ]");
	else ImGui::TextDisabled("[ B ]");
	ImGui::SameLine();

	if (GetButton(XINPUT_GAMEPAD_X)) ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "[ X ]");
	else ImGui::TextDisabled("[ X ]");
	ImGui::SameLine();

	if (GetButton(XINPUT_GAMEPAD_Y)) ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "[ Y ]");
	else ImGui::TextDisabled("[ Y ]");

	ImGui::End();
#endif
}