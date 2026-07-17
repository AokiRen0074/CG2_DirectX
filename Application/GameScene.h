#pragma once
#include "3D/Object3d.h"
#include "2D/Sprite.h"
#include "3D/DebugCamera.h"
#include "Audio/Audio.h"
#include "ViewProjection.h"
#include "Player.h"


class DirectXCommon;

class GameScene {
public:

    void Initialize(DirectXCommon* dxCommon);
    void Update();
    void Draw();

    ~GameScene();

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

    int currentModelType_ = 0;
    const char* modelNames_[4] = { "Plane&sprite","Teapot","Bunny","Suzanne" };

    /*----------------------
    ティーポット
    -----------------------------*/
    Object3d* teapot_ = nullptr;
    uint32_t teapotTexture_ = 0u;

    /*--------------------------
    バニー
    -----------------------------*/
    Object3d* bunny_ = nullptr;

    /*---------------------------
    スザンヌ
    ------------------------------*/
    Object3d* suzanne_ = nullptr;

};
