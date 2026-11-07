#pragma once
#include "Base/DirectXCommon.h"
#include "Scene/SceneManager.h"
#include <memory>

class GameApplication {
public:
    GameApplication() = default;
    ~GameApplication();
    int Run(const char* commandLine);
    GameApplication(const GameApplication&) = delete;
    GameApplication& operator=(const GameApplication&) = delete;

private:
    void Initialize();
    void MainLoop();
    void Finalize() noexcept;

    std::unique_ptr<DirectXCommon> dxCommon_;
    SceneManager sceneManager_;
    bool comInitialized_ = false;
    bool loggerInitialized_ = false;
    bool windowInitialized_ = false;
};
