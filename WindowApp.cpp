#include "WindowApp.h"
#include <dbghelp.h>
#include <strsafe.h>

#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "d3d12.lib") 
#pragma comment(lib, "dxgi.lib")

/*-----------------------
インスタンスの生成
--------------------------*/
WindowApp* WindowApp::GetInstance() {
    static WindowApp instance;
    return &instance;
}

/*--------------------------
ウィンドウプロシージャ
------------------------------*/
LRESULT CALLBACK WindowApp::WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

/*--------------------------
ダンプ出力
------------------------------*/
LONG WINAPI WindowApp::ExportDump(EXCEPTION_POINTERS* exception) {
    SYSTEMTIME time;
    GetLocalTime(&time);
    wchar_t filePath[MAX_PATH] = { 0 };
    CreateDirectory(L"./Dumps", nullptr);
    StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
    HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

    DWORD processId = GetCurrentProcessId();
    DWORD threadId = GetCurrentThreadId();

    MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
    minidumpInformation.ThreadId = threadId;
    minidumpInformation.ExceptionPointers = exception;
    minidumpInformation.ClientPointers = TRUE;

    MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);
    CloseHandle(dumpFileHandle);

    return EXCEPTION_EXECUTE_HANDLER;
}

/*-------------------------------
初期化 ウィンドウの生成
----------------------------*/
void WindowApp::Initialize() {
    SetUnhandledExceptionFilter(ExportDump);

    wc_.lpfnWndProc = WindowProc;
    wc_.lpszClassName = L"CG2";
    wc_.hInstance = GetModuleHandle(nullptr);
    wc_.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClass(&wc_);

    RECT wrc{ 0,0,kClientWidth,kClientHeight };
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

    hwnd_ = CreateWindow(
        wc_.lpszClassName, L"CG2", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wrc.right - wrc.left, wrc.bottom - wrc.top,
        nullptr, nullptr, wc_.hInstance, nullptr
    );


    // デバッグレイヤー
#ifdef _DEBUG

    if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController_)))) {
		// デバッグレイヤーを有効化
        debugController_->EnableDebugLayer();
        // GPU側でもチェック
		debugController_->SetEnableGPUBasedValidation(true);
	}

#endif

    ShowWindow(hwnd_, SW_SHOW);
}

/*--------------------------
終了処理
-----------------------*/
void WindowApp::Finalize() {
    CloseWindow(hwnd_);
}

/*----------------------------
ウィンドウメッセージの処理
---------------------------*/
bool WindowApp::ProcessMessage() {
    MSG msg{};
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (msg.message == WM_QUIT) {
        return true; // 終了メッセージを受け取った
    }

    return false;
}