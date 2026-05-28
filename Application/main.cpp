#include "Base/Logger.h"
#include "Base/WindowApp.h"
#include "Base/DirectXCommon.h"
#include "Input/Input.h"
#include "Audio/Audio.h"
#include "GameScene.h" 
#include <format>
#include <dxgidebug.h>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#endif

#pragma comment(lib,"dxguid.lib")

struct D3DResourceLeakChecker {
    ~D3DResourceLeakChecker() {
        Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
            debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        }
    }
};

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
    D3DResourceLeakChecker leakCheck;
    CoInitializeEx(0, COINIT_MULTITHREADED);

    // ==========================================
    // エンジンの初期化
    // ==========================================
    Logger::Initialize();
    WindowApp* winApp = WindowApp::GetInstance();
    winApp->Initialize();

    DirectXCommon* dxCommon = new DirectXCommon();
    dxCommon->Initialize(winApp);

    Input::GetInstance()->Initialize(winApp->GetHInstance(), winApp->GetHwnd());

    Audio::GetInstance()->Initialize();

    // ==========================================
    // ゲームシーンの初期化
    // ==========================================
    GameScene* gameScene = new GameScene();
    gameScene->Initialize(dxCommon);

    // ==========================================
    // メインループ
    // ==========================================
    while (true) {
        if (winApp->ProcessMessage()) { break; }

        // --- システム更新 ---
        Input::GetInstance()->Update();

#ifdef USE_IMGUI
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
#endif

        // --- ゲームの更新 ---
        gameScene->Update();

#ifdef USE_IMGUI
        ImGui::Render();
#endif

        // --- 描画処理 ---
        dxCommon->PreDraw();
        gameScene->Draw();
        dxCommon->PostDraw();
    }

    // ==========================================
    // 終了処理
    // ==========================================
    delete gameScene; // ゲームのデータを解放

    delete dxCommon;
    Audio::GetInstance()->Finalize();

    CoUninitialize();

#ifdef USE_IMGUI
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
#endif

    winApp->Finalize();
    Logger::Finalize();

    return 0;
}