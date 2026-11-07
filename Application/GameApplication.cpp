#include "GameApplication.h"
#include "GameScene.h"
#include "Base/Logger.h"
#include "Base/WindowApp.h"
#include "Input/Input.h"
#include "Audio/Audio.h"
#include "2D/Sprite.h"
#include "2D/TextureManager.h"
#include "3D/ModelManager.h"
#include <dxgidebug.h>
#include <stdexcept>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#endif

#pragma comment(lib, "dxguid.lib")

namespace {
struct D3DResourceLeakChecker {
    ~D3DResourceLeakChecker() {
        Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
            debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        }
    }
};
}

GameApplication::~GameApplication() { Finalize(); }

int GameApplication::Run(const char* /*commandLine*/) {
    D3DResourceLeakChecker leakCheck;
    try {
        Initialize();
        MainLoop();
        Finalize();
        return 0;
    } catch (const std::exception& error) {
        Logger::Log(error.what());
        Logger::Log("\n");
        Finalize();
        return 1;
    }
}

void GameApplication::Initialize() {
    const HRESULT result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(result)) throw std::runtime_error("Failed to initialize COM");
    comInitialized_ = true;
    Logger::Initialize();
    loggerInitialized_ = true;
    auto* window = WindowApp::GetInstance();
    window->Initialize();
    windowInitialized_ = true;
    dxCommon_ = std::make_unique<DirectXCommon>();
    dxCommon_->Initialize(window);
    Sprite::StaticInitialize(dxCommon_.get());
    Object3d::StaticInitialize(dxCommon_.get());
    TextureManager::StaticInitialize(dxCommon_.get());
    Input::GetInstance()->Initialize(window->GetHInstance(), window->GetHwnd());
    Audio::GetInstance()->Initialize();
    auto scene = std::make_unique<GameScene>();
    scene->Initialize(dxCommon_.get());
    sceneManager_.ChangeScene(std::move(scene));
}

void GameApplication::MainLoop() {
    while (!WindowApp::GetInstance()->ProcessMessage()) {
        Input::GetInstance()->Update();
#ifdef USE_IMGUI
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
#endif
        sceneManager_.Update();
#ifdef USE_IMGUI
        ImGui::Render();
#endif
        dxCommon_->PreDraw();
        sceneManager_.Draw();
        dxCommon_->PostDraw();
    }
}

void GameApplication::Finalize() noexcept {
    sceneManager_.Clear();
#ifdef USE_IMGUI
    // デバイスより先に ImGui の描画リソースを解放する。
    if (ImGui::GetCurrentContext()) {
        if (ImGui::GetIO().BackendRendererUserData) ImGui_ImplDX12_Shutdown();
        if (ImGui::GetIO().BackendPlatformUserData) ImGui_ImplWin32_Shutdown();
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
