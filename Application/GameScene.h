#pragma once
#include "Scene/IScene.h"
#include "3D/Object3d.h"
#include "2D/Sprite.h"
#include "3D/DebugCamera.h"
#include "Audio/Audio.h"
#include "ViewProjection.h"
#include "Player.h"


class DirectXCommon;

class GameScene : public IScene {
public:

    void Initialize(DirectXCommon* dxCommon);
    void Update() override;
    void Draw() override;

    ~GameScene() override;

private:

    /*--------------------
    自キャラ
    ------------------------*/
    Player* player_ = nullptr;

    ModelData* modelData_ = nullptr;
    uint32_t textureHandle_=0u;

    // ゲームで使うカメラ
    DebugCamera* debugCamera_ = nullptr;

    // ゲームで使うオブジェクト達
    Object3d* object3d_ = nullptr;
    Sprite* sprite_ = nullptr;

    // ビュープロジェクション
    ViewProjection viewProjection_;

    // 音声データ
    SoundData soundData_;
};
