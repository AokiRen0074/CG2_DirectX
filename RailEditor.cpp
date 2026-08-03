#include "RailEditor.h"
#include "MathUtility.h" 
#include "Input/Input.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void RailEditor::Initialize(Rail* rail) {
    rail_ = rail;

    // 制御点を可視化するためのモデル
    pointModel_ = new Object3d();
    pointModel_->Initialize("Resources", "block.obj");

    // レールの制御点の数だけワールドトランスフォームを用意する
    pointTransforms_.resize(rail_->GetControlPoints().size());
    for (auto& transform : pointTransforms_) {
        transform.Initialize();
        transform.scale_ = { 2.0f, 2.0f, 2.0f }; // 掴みやすいように少し大きめにする
    }
}

void RailEditor::Update(DebugCamera* camera) {
    if (!rail_ || !camera) return;

    Input* input = Input::GetInstance();
    auto& points = rail_->GetControlPoints(); // 制御点を参照で取得

    // ImGuiの機能を使って、OSのマウス座標を安全に取得
    float mouseX = 0.0f;
    float mouseY = 0.0f;
#ifdef USE_IMGUI
    ImVec2 mousePos = ImGui::GetMousePos();
    mouseX = mousePos.x;
    mouseY = mousePos.y;
#endif

    // =================================================
    // マウス座標から3D空間へ向かうレイを計算
    // =================================================
    // NDC 
    float nx = (2.0f * mouseX) / 1280.0f - 1.0f;
    float ny = 1.0f - (2.0f * mouseY) / 720.0f;

    // ViewProjection の逆行列を計算して画面からワールド空間へ逆算
    Matrix4x4 matVP = Multiply(camera->GetViewMatrix(), camera->GetProjectionMatrix());
    Matrix4x4 matInvVP = Inverse(matVP);

    float w = nx * matInvVP.m[0][3] + ny * matInvVP.m[1][3] + 1.0f * matInvVP.m[2][3] + 1.0f * matInvVP.m[3][3];
    Vector3 worldPos = {
        (nx * matInvVP.m[0][0] + ny * matInvVP.m[1][0] + 1.0f * matInvVP.m[2][0] + 1.0f * matInvVP.m[3][0]) / w,
        (nx * matInvVP.m[0][1] + ny * matInvVP.m[1][1] + 1.0f * matInvVP.m[2][1] + 1.0f * matInvVP.m[3][1]) / w,
        (nx * matInvVP.m[0][2] + ny * matInvVP.m[1][2] + 1.0f * matInvVP.m[2][2] + 1.0f * matInvVP.m[3][2]) / w
    };

    // カメラの現在位置
    Matrix4x4 matInvView = Inverse(camera->GetViewMatrix());
    Vector3 rayOrigin = { matInvView.m[3][0], matInvView.m[3][1], matInvView.m[3][2] };

    // レイの方向ベクトル
    Vector3 rayDir = Normalize(worldPos - rayOrigin);

    // =================================================
    // マウスの左クリックで制御点を掴む・動かす
    // =================================================
    if (input->PushMouseLeft()) {
        if (grabbedIndex_ == -1) {
            // まだ何も掴んでいない場合、レイと各制御点のを行う
            float closestDist = 99999.0f;
            float hitRadius = 4.0f; // 掴める判定の広さ

            for (size_t i = 0; i < points.size(); ++i) {
                Vector3 toPoint = points[i] - rayOrigin;
                float t = Dot(toPoint, rayDir); // レイ上の最も近い点の距離

                if (t > 0.0f) {
                    Vector3 closestPoint = rayOrigin + rayDir * t;
                    float distanceToRay = Length(points[i] - closestPoint); // レイと制御点の最短距離

                    // レイの近くにあり、かつ最も手前にある点を見つける
                    if (distanceToRay < hitRadius && t < closestDist) {
                        closestDist = t;
                        grabbedIndex_ = (int)i;
                        grabbedDistance_ = t; // 掴んだ時の「奥行き」を記憶
                    }
                }
            }
        }
        else {
            // 既に掴んでいる場合、マウスの動きに合わせて座標を更新 
            points[grabbedIndex_] = rayOrigin + rayDir * grabbedDistance_;
        }
    }
    else {
        // 左クリックを離したら掴むのを解除
        grabbedIndex_ = -1;
    }

    // =================================================
    // 描画用の座標を更新
    // =================================================
    for (size_t i = 0; i < points.size(); ++i) {
        pointTransforms_[i].translation_ = points[i];
        pointTransforms_[i].matWorld_ = MakeAffineMatrix(pointTransforms_[i].scale_, pointTransforms_[i].rotation_, pointTransforms_[i].translation_);
        pointTransforms_[i].TransferMatrix();
    }
}

void RailEditor::Draw(const ViewProjection& viewProjection) {
    if (!pointModel_) return;

    // テクスチャなし用の描画関数で制御点を表示
    for (size_t i = 0; i < pointTransforms_.size(); ++i) {
        pointModel_->Draw(pointTransforms_[i], viewProjection);
    }
}