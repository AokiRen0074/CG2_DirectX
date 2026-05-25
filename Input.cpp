#include "Input.h"
#include <cassert>

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