#pragma once

#define DIRECTINPUT_VERSION 0x0800 
#include <dinput.h>
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

#include <wrl.h>
#include <stdint.h> 

class Input {
public:
	// 初期化
	void Initialize(HINSTANCE hInstance, HWND hwnd);

	// 毎フレームの更新
	void Update();


	bool PushKey(uint8_t keyNum) const;       // キーを押した状態か
	bool NotPushKey(uint8_t keyNum) const;    // キーを離した状態か
	bool TriggerKey(uint8_t keyNum) const;    // キーを押した瞬間か
	bool ReleaseKey(uint8_t keyNum) const;    // キーを離した瞬間か

private:
	Microsoft::WRL::ComPtr<IDirectInput8> directInput_;
	Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard_;

	BYTE key_[256] = {};     // 今フレームのキー状態
	BYTE keyPre_[256] = {};  // 前フレームのキー状態
};