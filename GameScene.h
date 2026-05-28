#pragma once
#include "3D/Object3d.h"
#include "2D/Sprite.h"
#include "3D/DebugCamera.h"
#include "Audio/Audio.h"


class DirectXCommon;

class GameScene {
public:

    void Initialize(DirectXCommon* dxCommon);
    void Update();
    void Draw();

    ~GameScene();

private:
    // ゲームで使うカメラ
    DebugCamera* debugCamera_ = nullptr;

    // ゲームで使うオブジェクト達
    Object3d* object3d_ = nullptr;
    Sprite* sprite_ = nullptr;

    // 音声データ
    SoundData soundData_;
};