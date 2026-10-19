#include "Base/Logger.h"
#include "Base/WindowApp.h"
#include "Base/DirectXCommon.h"
#include "Input/Input.h"
#include "Audio/Audio.h"
#include "GameScene.h" 
#include <format>
#include <cstring>
#include "Character/SpearCapture.h"
#include <dxgidebug.h>
#include "2D/TextureManager.h"
#include "3D/ModelManager.h"

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
            //debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        }
    }
};

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR commandLine, _In_ int) {
    const bool spearEffectSequence = std::strstr(commandLine, "--spear-effect-sequence") != nullptr;
    const bool spearEditorSmokeTest = std::strstr(commandLine, "--spear-editor-smoke-test") != nullptr;
    const bool spearPierceSmokeTest = std::strstr(commandLine, "--spear-pierce-smoke-test") != nullptr;
    const bool spearDodgeSmokeTest = std::strstr(commandLine, "--spear-dodge-smoke-test") != nullptr;
    const bool spearBeamSmokeTest = std::strstr(commandLine, "--spear-beam-smoke-test") != nullptr;
    const bool spearLaserSmokeTest = std::strstr(commandLine, "--spear-laser-smoke-test") != nullptr;
    const bool spearSummonSmokeTest = std::strstr(commandLine, "--spear-summon-smoke-test") != nullptr;
    const bool spearVictorySmokeTest = std::strstr(commandLine, "--spear-victory-smoke-test") != nullptr;
    const bool spearBossSmokeTest = std::strstr(commandLine, "--spear-boss-smoke-test") != nullptr;
    const std::string arguments = commandLine;
    const size_t captureArgument = arguments.find("--spear-capture=");
    const std::string capturePath = captureArgument == std::string::npos ? "" :
        arguments.substr(captureArgument + std::strlen("--spear-capture="));
    bool capturePassed = true;
    const bool spearSmokeTest = spearPierceSmokeTest || spearDodgeSmokeTest || spearBeamSmokeTest || spearLaserSmokeTest || spearSummonSmokeTest || spearVictorySmokeTest || spearBossSmokeTest || spearEditorSmokeTest || std::strstr(commandLine, "--spear-smoke-test") != nullptr;
    int smokeFrames = 0;

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

    // Spriteクラスに共通のdxCommonを教える
    Sprite::StaticInitialize(dxCommon);


    TextureManager::StaticInitialize(dxCommon);

    Input::GetInstance()->Initialize(winApp->GetHInstance(), winApp->GetHwnd());

    Audio::GetInstance()->Initialize();



    // ==========================================
    // ゲームシーンの初期化
    // ==========================================
    GameScene* gameScene = new GameScene();
    gameScene->Initialize(dxCommon);
    if (spearEffectSequence) {
        gameScene->SetFixedPreviewStep();
    }

    if (spearLaserSmokeTest || spearSummonSmokeTest || spearBeamSmokeTest) {
        gameScene->PreviewBossAttack(spearLaserSmokeTest ? 0 : spearSummonSmokeTest ? 1 : 2);
    }

    if (spearBossSmokeTest || spearVictorySmokeTest) {
        gameScene->OpenBossPractice();
    }

    if (spearEditorSmokeTest) {
        gameScene->OpenSpearEditor();
    }


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
        // 実際の攻撃判定で撃破を発生させ、ブラーを画面で確認する。
        if (spearVictorySmokeTest && smokeFrames == 98) {
            gameScene->PreviewBossDefeat();
        }

        if (spearPierceSmokeTest && smokeFrames == 94) {
            gameScene->PreviewPierce();
        }

        if (spearDodgeSmokeTest && smokeFrames == 98) {
            gameScene->PreviewDodge();
        }

        gameScene->Update();

#ifdef USE_IMGUI
        ImGui::Render();
#endif

        // --- 描画処理 ---
        dxCommon->PreDraw();
        gameScene->Draw();
        const UINT captureIndex = dxCommon->GetSwapChain()->GetCurrentBackBufferIndex();
        dxCommon->PostDraw();
        ++smokeFrames;

        if (!spearEffectSequence && !capturePath.empty() && smokeFrames == (spearPierceSmokeTest ? 101 : (spearVictorySmokeTest || spearDodgeSmokeTest ? 100 : 110))) {
            capturePassed = CaptureSpearFrame(dxCommon, captureIndex, capturePath);
        }


        // 演出の出始めから消えるまでを、固定更新で連続確認する開発用モード。
        if (spearEffectSequence && !capturePath.empty() && smokeFrames >= ((spearBeamSmokeTest || spearLaserSmokeTest) ? 1 : 94) && smokeFrames <= ((spearBeamSmokeTest || spearLaserSmokeTest) ? 180 : 150)) {
            const auto framePath = std::filesystem::path(capturePath) /
                std::format("frame-{:03}.png", smokeFrames);
            capturePassed = CaptureSpearFrame(dxCommon, captureIndex, framePath) && capturePassed;
        }

        // 起動・描画・終了までを短時間で確認する開発用モード。
        if (spearSmokeTest && smokeFrames >= (spearEffectSequence ? ((spearBeamSmokeTest || spearLaserSmokeTest) ? 180 : 150) : 120)) {
            break;
        }

    }

    // ==========================================
    // 終了処理
    // ==========================================
    delete gameScene; // ゲームのデータを解放
#ifdef USE_IMGUI
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
#endif
	ModelManager::Finalize();
	TextureManager::Finalize(); 
    delete dxCommon;
    Audio::GetInstance()->Finalize();

    CoUninitialize();


    winApp->Finalize();
    Logger::Finalize();

    return capturePassed ? 0 : 1;
}
