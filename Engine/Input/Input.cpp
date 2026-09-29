#include "Input.h"
#include <cassert>



Input* Input::GetInstance() {
	static Input instance;
	return &instance;
}

/*-----------------------------------
初期化
---------------------------------*/
void Input::Initialize(HINSTANCE hInstance, HWND hwnd) {
	HRESULT result;

	// DirectInputオブジェクトの生成
	result = DirectInput8Create(
		hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8,
		(void**)&directInput_, nullptr);
	assert(SUCCEEDED(result));

	// キーボードデバイスの生成
	result = directInput_->CreateDevice(GUID_SysKeyboard, &keyboard_, NULL);
	assert(SUCCEEDED(result));

	// 入力データ形式のセット
	result = keyboard_->SetDataFormat(&c_dfDIKeyboard);
	assert(SUCCEEDED(result));

	// 排他制御レベルのセット
	result = keyboard_->SetCooperativeLevel(
		hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
	assert(SUCCEEDED(result));

	result = directInput_->CreateDevice(GUID_SysMouse, &mouse_, NULL);
	assert(SUCCEEDED(result));

	result = mouse_->SetDataFormat(&c_dfDIMouse);
	assert(SUCCEEDED(result));

	result = mouse_->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	assert(SUCCEEDED(result));
}
/*-------------------------
更新
-------------------------------------*/
void Input::Update() {
	//今のキー状態を、前フレームのキー状態として丸ごとコピーして保存する
	memcpy(keyPre_, key_, sizeof(key_));

	// キーボード情報の取得開始
	keyboard_->Acquire();

	// 全キーの入力状態を取得する
	keyboard_->GetDeviceState(sizeof(key_), key_);

	mouseStatePre_ = mouseState_; // 前フレームの状態を保存
	mouse_->Acquire();
	mouse_->GetDeviceState(sizeof(DIMOUSESTATE), &mouseState_);
}


// キーを押した状態か
bool Input::PushKey(uint8_t keyNum) const {
	return key_[keyNum] != 0;
}

// キーを離した状態か
bool Input::NotPushKey(uint8_t keyNum) const {
	return key_[keyNum] == 0;
}

// キーを押した瞬間か
bool Input::TriggerKey(uint8_t keyNum) const {
	return (key_[keyNum] != 0) && (keyPre_[keyNum] == 0);
}

// キーを離した瞬間か
bool Input::ReleaseKey(uint8_t keyNum) const {
	return (key_[keyNum] == 0) && (keyPre_[keyNum] != 0);
}

// ==========================================
// マウスの移動量を取得
// ==========================================

float Input::GetMouseMoveX() const {
	// lX がX方向の移動量
	return (float)mouseState_.lX;
}

float Input::GetMouseMoveY() const {
	// lY がY方向の移動量
	return (float)mouseState_.lY;
}

bool Input::PushMouseLeft() const {
	// 押されていれば 0x80 が立つので判定する
	return (mouseState_.rgbButtons[0] & 0x80) != 0;
}

bool Input::PushMouseRight() const {
	return (mouseState_.rgbButtons[1] & 0x80) != 0;
}

bool Input::PushMouseMiddle() const {
	return (mouseState_.rgbButtons[2] & 0x80) != 0;
}

float Input::GetWheel() const {
	return (float)mouseState_.lZ;
}