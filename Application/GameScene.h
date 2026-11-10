#pragma once
#include "Scene/IScene.h"
#include "3D/Object3d.h"
#include "3D/DebugCamera.h"
#include "3D/ViewProjection.h"
#include "Character/Player.h"
#include <memory>

class DirectXCommon;

class GameScene : public IScene {
public:
    void Initialize(DirectXCommon* dxCommon);
    void Update() override;
    void Draw() override;
    ~GameScene() override = default;

private:
    // メンバは宣言の逆順で破棄されるため、Playerよりモデルを先に宣言する。
    std::unique_ptr<Object3d> object3d_;
    std::unique_ptr<DebugCamera> debugCamera_;
    std::unique_ptr<Player> player_;
    ViewProjection viewProjection_;
};
