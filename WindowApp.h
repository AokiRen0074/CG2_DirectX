#pragma once
#include <Windows.h>
#include <cstdint>
#include <wrl.h>
#include <d3d12.h>

class WindowApp {
public:
	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

public:
	// シングルトン化
	static WindowApp* GetInstance();

	// 初期化とウィンドウの生成
	void Initialize();
	// 終了処理
	void Finalize();

	// ウィンドウメッセージの処理
	bool ProcessMessage();

	// ゲッター
	HWND GetHwnd() const { return hwnd_; }

private:
	WindowApp() = default;
	~WindowApp() = default;
	WindowApp(const WindowApp&) = delete;
	WindowApp& operator=(const WindowApp&) = delete;

	// ウィンドウプロシージャ
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	// ダンプ出力
	static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);

private:
	HWND hwnd_ = nullptr;
	WNDCLASS wc_{};

	Microsoft::WRL::ComPtr<ID3D12Debug1> debugController_;

};