#include "GameApplication.h"
#include "SpearSceneStates.h"
#include "AxisIndicator.h"
#include <stdexcept>
#include "Base/Logger.h"
#include "Base/WindowApp.h"
#include "Base/DirectXCommon.h"
#include "Input/Input.h"
#include "Audio/Audio.h"
#include "GameScene.h" 
#include <format>
#include <cstring>
#include <fstream>
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

GameApplication::~GameApplication() {
	Finalize();
}

int GameApplication::Run(const char* commandLine) {
	D3DResourceLeakChecker leakCheck;

	try {
		const int result = RunInternal(commandLine ? commandLine : "");
		Finalize();
		return result;
	}
	catch (const std::exception& error) {
		Logger::Log(error.what());
		Logger::Log("\n");
		Finalize();
		return 1;
	}
}

void GameApplication::Finalize() noexcept {
    // ==========================================
    // 終了処理
    // ==========================================
    sceneManager_.Clear();
    gameScene_.reset(); // ゲームのデータを解放
    AxisIndicator::GetInstance()->Finalize();

#ifdef USE_IMGUI
    if (ImGui::GetCurrentContext()) {
        if (ImGui::GetIO().BackendRendererUserData) {
            ImGui_ImplDX12_Shutdown();
        }

        if (ImGui::GetIO().BackendPlatformUserData) {
            ImGui_ImplWin32_Shutdown();
        }

        ImGui::DestroyContext();
    }
#endif

	ModelManager::Finalize();
	TextureManager::Finalize();
	Audio::GetInstance()->Finalize();
	dxCommon_.reset();

	if (windowInitialized_) {
		WindowApp::GetInstance()->Finalize();
		windowInitialized_ = false;
	}

	if (comInitialized_) {
		CoUninitialize();
		comInitialized_ = false;
	}

	if (loggerInitialized_) {
		Logger::Finalize();
		loggerInitialized_ = false;
	}
}

int GameApplication::RunInternal(const char* commandLine) {
    const bool spearUiInteractionTest = std::strstr(commandLine, "--spear-ui-interaction-test") != nullptr;
    const bool spearEffectSequence = std::strstr(commandLine, "--spear-effect-sequence") != nullptr;
    const bool spearUiEditorSmokeTest = std::strstr(commandLine, "--spear-ui-editor-smoke-test") != nullptr;
    const bool spearUiClearPreview = std::strstr(commandLine, "--spear-ui-clear-preview") != nullptr;
    const bool spearLoopSmokeTest = std::strstr(commandLine, "--spear-loop-smoke-test") != nullptr;
    const bool spearEditorSmokeTest = std::strstr(commandLine, "--spear-editor-smoke-test") != nullptr;
    const bool spearPierceSmokeTest = std::strstr(commandLine, "--spear-pierce-smoke-test") != nullptr;
    const bool spearDodgeSmokeTest = std::strstr(commandLine, "--spear-dodge-smoke-test") != nullptr;
    const bool spearBeamSmokeTest = std::strstr(commandLine, "--spear-beam-smoke-test") != nullptr;
    const bool spearLaserSmokeTest = std::strstr(commandLine, "--spear-laser-smoke-test") != nullptr;
    const bool spearSummonSmokeTest = std::strstr(commandLine, "--spear-summon-smoke-test") != nullptr;
    const bool spearVictorySmokeTest = std::strstr(commandLine, "--spear-victory-smoke-test") != nullptr;
    const bool spearBossSmokeTest = std::strstr(commandLine, "--spear-boss-smoke-test") != nullptr;
    const bool spearCounterSmokeTest = std::strstr(commandLine, "--spear-counter-smoke-test") != nullptr;
    const bool spearNovaSmokeTest = std::strstr(commandLine, "--spear-nova-smoke-test") != nullptr;
    const bool spearFanSmokeTest = std::strstr(commandLine, "--spear-fan-smoke-test") != nullptr;
    const bool spearRainSmokeTest = std::strstr(commandLine, "--spear-rain-smoke-test") != nullptr;
    const bool spearPhaseSmokeTest = std::strstr(commandLine, "--spear-phase-smoke-test") != nullptr;
    const bool spearComboSmokeTest = std::strstr(commandLine, "--spear-combo-smoke-test") != nullptr;
    const bool extendedBossSequence = spearBeamSmokeTest || spearLaserSmokeTest || spearFanSmokeTest ||
                                     spearRainSmokeTest || spearPhaseSmokeTest || spearComboSmokeTest;
    const std::string arguments = commandLine;
    const size_t captureArgument = arguments.find("--spear-capture=");
    const std::string capturePath = captureArgument == std::string::npos ? "" :
        arguments.substr(captureArgument + std::strlen("--spear-capture="));
    bool capturePassed = true;
    const bool spearSmokeTest = spearCounterSmokeTest || spearNovaSmokeTest || spearUiInteractionTest || spearUiEditorSmokeTest || spearLoopSmokeTest || spearFanSmokeTest || spearRainSmokeTest || spearPhaseSmokeTest || spearComboSmokeTest || spearPierceSmokeTest || spearDodgeSmokeTest || spearBeamSmokeTest || spearLaserSmokeTest || spearSummonSmokeTest || spearVictorySmokeTest || spearBossSmokeTest || spearEditorSmokeTest || std::strstr(commandLine, "--spear-smoke-test") != nullptr;
    int smokeFrames = 0;
    SpearUiLayout uiTestBaseline;
    std::array<float,4> uiTestRect{};
    if (spearUiInteractionTest) {
        std::ifstream layout("Resources/UI/PrismUI/layout.json");
        nlohmann::json json;
        layout >> json;
        uiTestBaseline = SpearUiLayout::Parse(json);
        SpearUiContext preview; preview.preview = "clear";
        for (const auto &draw : uiTestBaseline.Resolve(preview)) {
            if (uiTestBaseline.elements[draw.element].id == "result_heading")
                uiTestRect = {10 + draw.x * 900 / 1280,104 + draw.y * 900 / 1280,
                              draw.width * 900 / 1280,draw.height * 900 / 1280};
        }
    }
    const auto verifyUiSave = [&](bool changed) {
        try {
            std::ifstream saved("Settings/ui-editor-smoke.json");
            nlohmann::json json;
            saved >> json;
            const auto layout = SpearUiLayout::Parse(json);
            for (size_t i = 0; i < layout.elements.size(); ++i) {
                const auto &e = layout.elements[i];
                if (e.id != "result_heading") {
                    continue;
                }
                const auto &original = uiTestBaseline.elements[i];
                const float dx = changed ? (50.0f + 42.0f * original.pivot[0]) / (900.0f / 1280) : 0;
                const float dy = changed ? 30.0f / (900.0f / 1280) : 0;
                const float dw = changed ? 42.0f / (900.0f / 1280) : 0;
                const float dh = changed ? 28.0f / (900.0f / 1280) : 0;
                return std::abs(e.position[0] - original.position[0] - dx) < 2 &&
                    std::abs(e.position[1] - original.position[1] - dy) < 2 &&
                    std::abs(e.size[0] - original.size[0] - dw) < 2 &&
                    std::abs(e.size[1] - original.size[1] - dh) < 2;
            }
        }
        catch (const std::exception&) {
            return false;
        }
        return false;
    };

    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    if (FAILED(comResult)) {
        throw std::runtime_error("Failed to initialize COM");
    }

    comInitialized_ = true;

    // ==========================================
    // エンジンの初期化
    // ==========================================
    Logger::Initialize();
    loggerInitialized_ = true;
    WindowApp* winApp = WindowApp::GetInstance();
    winApp->Initialize();
    windowInitialized_ = true;

    dxCommon_ = std::make_unique<DirectXCommon>();
    DirectXCommon* dxCommon = dxCommon_.get();
    dxCommon->Initialize(winApp);

    // Spriteクラスに共通のdxCommonを教える
    Sprite::StaticInitialize(dxCommon);


    TextureManager::StaticInitialize(dxCommon);

    Input::GetInstance()->Initialize(winApp->GetHInstance(), winApp->GetHwnd());

    Audio::GetInstance()->Initialize();



    // ==========================================
    // ゲームシーンの初期化
    // ==========================================
    gameScene_ = std::make_shared<GameScene>();
    GameScene* gameScene = gameScene_.get();
    gameScene->Initialize(dxCommon);
    sceneManager_.ChangeScene(std::make_unique<PlayingSceneState>(gameScene_, sceneManager_));
    if (spearEffectSequence || spearLoopSmokeTest || spearNovaSmokeTest || spearCounterSmokeTest) {
        gameScene->SetFixedPreviewStep();
    }

    if (spearLaserSmokeTest || spearSummonSmokeTest || spearBeamSmokeTest) {
        gameScene->PreviewBossAttack(spearLaserSmokeTest ? 0 : spearSummonSmokeTest ? 1 : 2);
    }

    if (spearNovaSmokeTest) {
        gameScene->PreviewBossAttack(7);
    }
    if (spearCounterSmokeTest) {
        gameScene->PreviewBossAttack(8);
    }

    if (spearFanSmokeTest || spearRainSmokeTest || spearPhaseSmokeTest || spearComboSmokeTest) {
        gameScene->PreviewBossAttack(spearFanSmokeTest ? 3 : spearRainSmokeTest ? 4 : spearPhaseSmokeTest ? 5 : 6);
    }

    if (spearBossSmokeTest || spearVictorySmokeTest) {
        gameScene->OpenBossPractice();
    }

    if (std::strstr(commandLine, "--stage-editor") != nullptr) {
        gameScene->OpenStageEditor();
    }

    if (spearUiInteractionTest || spearUiEditorSmokeTest || std::strstr(commandLine, "--ui-editor") != nullptr) {
        gameScene->OpenUiEditor(spearUiClearPreview || spearUiInteractionTest ? "clear" : "title");
        if (spearUiInteractionTest) {
            gameScene->SetUiEditorTestSavePath();
        }
    }

    if (spearEditorSmokeTest) {
        gameScene->OpenSpearEditor();
    }


    // ==========================================
    // メインループ
    // ==========================================
    while (true) {
        if (winApp->ProcessMessage()) {
            break;
        }

        // --- システム更新 ---
        Input::GetInstance()->Update();

#ifdef USE_IMGUI
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        if (spearUiInteractionTest) {
            auto &io = ImGui::GetIO();
            // Feed the real editor widgets through ImGui's input queue.
            const auto pressMouse = [&](float x, float y) {
                io.AddMousePosEvent(x, y);
                io.AddMouseButtonEvent(0, true);
            };

            if (smokeFrames == 20 || smokeFrames == 30) {
                pressMouse(uiTestRect[0] + uiTestRect[2] * 0.5f,
                           uiTestRect[1] + uiTestRect[3] * 0.5f);
            }

            if (smokeFrames == 32) {
                io.AddMousePosEvent(uiTestRect[0] + uiTestRect[2] * 0.5f + 50,
                                   uiTestRect[1] + uiTestRect[3] * 0.5f + 30);
            }

            if (smokeFrames == 50) {
                pressMouse(uiTestRect[0] + uiTestRect[2] + 50,
                           uiTestRect[1] + uiTestRect[3] + 30);
            }

            if (smokeFrames == 52) {
                io.AddMousePosEvent(uiTestRect[0] + uiTestRect[2] + 92,
                                   uiTestRect[1] + uiTestRect[3] + 58);
            }

            if (smokeFrames == 60 || smokeFrames == 115) {
                pressMouse(65, 46);
            }

            if (smokeFrames == 90 || smokeFrames == 100) {
                pressMouse(300, 46);
            }

            if (smokeFrames == 22 || smokeFrames == 34 || smokeFrames == 54 ||
                smokeFrames == 62 || smokeFrames == 92 || smokeFrames == 102 ||
                smokeFrames == 117) {
                io.AddMouseButtonEvent(0, false);
            }

            if (smokeFrames == 80) {
                capturePassed = verifyUiSave(true) && capturePassed;
            }

            if (smokeFrames == 130) {
                capturePassed = verifyUiSave(false) && capturePassed;
            }
        }
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

        if (spearLoopSmokeTest) {
            if (smokeFrames == 60) {
                gameScene->OpenBossPractice();
            }
            if (smokeFrames == 100) {
                gameScene->PreviewBossDefeat();
            }
            if (smokeFrames == 380) {
                capturePassed = gameScene->IsResultReady() && capturePassed;
                capturePassed = dynamic_cast<const ResultSceneState*>(
                    sceneManager_.GetCurrentScene()) != nullptr && capturePassed;
                Logger::Log("Scene state check: result\n");
                gameScene->RestartRun();
                capturePassed = !gameScene->IsResultReady() && capturePassed;
            }
            if (smokeFrames == 465) {
                capturePassed = !gameScene->IsResultReady() && capturePassed;
                capturePassed = dynamic_cast<const PlayingSceneState*>(
                    sceneManager_.GetCurrentScene()) != nullptr && capturePassed;
                Logger::Log("Scene state check: playing after restart\n");
            }
        }
        sceneManager_.Update();

#ifdef USE_IMGUI
        ImGui::Render();
#endif

        // --- 描画処理 ---
        dxCommon->PreDraw();
        sceneManager_.Draw();
        const UINT captureIndex = dxCommon->GetSwapChain()->GetCurrentBackBufferIndex();
        dxCommon->PostDraw();
        ++smokeFrames;

        if (!spearCounterSmokeTest && !spearNovaSmokeTest && !spearLoopSmokeTest && !spearEffectSequence && !capturePath.empty() && smokeFrames == (spearPierceSmokeTest ? 101 : (spearVictorySmokeTest || spearDodgeSmokeTest ? 100 : 110))) {
            capturePassed = CaptureSpearFrame(dxCommon, captureIndex, capturePath);
        }


        // 演出の出始めから消えるまでを、固定更新で連続確認する開発用モード。
        if (spearEffectSequence && !capturePath.empty() && smokeFrames >= (extendedBossSequence ? 1 : 94) && smokeFrames <= (extendedBossSequence ? 180 : 150)) {
            const auto framePath = std::filesystem::path(capturePath) /
                std::format("frame-{:03}.png", smokeFrames);
            capturePassed = CaptureSpearFrame(dxCommon, captureIndex, framePath) && capturePassed;
        }

        if (spearLoopSmokeTest && !capturePath.empty()) {
            const char *shot = smokeFrames == 20 ? "title.png" : smokeFrames == 90 ? "departed.png"
                             : smokeFrames == 103 ? "impact.png" : smokeFrames == 170 ? "aftermath.png"
                             : smokeFrames == 260 ? "chart.png" : smokeFrames == 300 ? "rank-impact.png"
                             : smokeFrames == 360 ? "clear.png" : smokeFrames == 406 ? "blackout.png"
                             : smokeFrames == 425 ? "fade-in.png" : smokeFrames == 455 ? "restart.png" : nullptr;
            if (shot) {
                capturePassed = CaptureSpearFrame(
                    dxCommon, captureIndex, std::filesystem::path(capturePath) / shot) && capturePassed;
            }
        }

        if (spearUiInteractionTest && !capturePath.empty() && (smokeFrames == 40 || smokeFrames == 56 || smokeFrames == 128)) {
            const auto image = std::filesystem::path(capturePath).parent_path() / std::format("interaction-{}.png",smokeFrames);
            capturePassed = CaptureSpearFrame(dxCommon,captureIndex,image) && capturePassed;
        }

        if (spearNovaSmokeTest && !capturePath.empty()) {
            const char *shot=smokeFrames==30?"nova-warning.png":smokeFrames==110?"nova-wave1.png":
                smokeFrames==160?"nova-wave2.png":smokeFrames==210?"nova-wave3.png":smokeFrames==275?"nova-recovery.png":nullptr;
            if (shot) {
                capturePassed = CaptureSpearFrame(
                    dxCommon, captureIndex, std::filesystem::path(capturePath) / shot) && capturePassed;
            }
        }
        if (spearCounterSmokeTest && !capturePath.empty()) {
            const char *shot=smokeFrames==4?"counter-hit.png":smokeFrames==18?"counter-shards.png":smokeFrames==45?"counter-opening.png":nullptr;
            if (shot) {
                capturePassed = CaptureSpearFrame(
                    dxCommon, captureIndex, std::filesystem::path(capturePath) / shot) && capturePassed;
            }
        }
        // 起動・描画・終了までを短時間で確認する開発用モード。
        if (spearSmokeTest && smokeFrames >= (spearCounterSmokeTest ? 80 : spearNovaSmokeTest ? 285 : spearUiInteractionTest ? 140 : spearLoopSmokeTest ? 470 : spearEffectSequence ? (extendedBossSequence ? 180 : 150) : 120)) {
            break;
        }

    }

    return capturePassed ? 0 : 1;
}
