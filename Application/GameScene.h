// GameScene.h
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




    DebugCamera* debugCamera_ = nullptr;
    Sprite* sprite_ = nullptr;

    // モード管理
    enum class SceneMode {
        Evaluation,  // 加点要素
        Presentation // 映像演出
    };
    SceneMode currentMode_ = SceneMode::Evaluation;

    // 共通のテクスチャ
    uint32_t texture1_ = 0;
    uint32_t texture2_ = 0;



    // ===================================
    //評価用の変数
    // ===================================
    Object3d* triangle1_ = nullptr;
    Object3d* triangle2_ = nullptr;
    int texIndex1_ = 0;
    int texIndex2_ = 1;



    // ===================================
    // 映像演出用の変数
    // ===================================
    struct TriangleParticle {
        Object3d* obj;
        Vector3 velocity;
        Vector3 rotSpeed;
    };
    static const int kNumParticles = 100;
    TriangleParticle particles_[kNumParticles];
};