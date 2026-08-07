#include "RailEditor.h"
#include "MathUtility.h" 
#include "Input/Input.h"
#include <cmath>
#include <cstdio>
#include "GlobalValiables.h"
#include <string>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void RailEditor::Initialize(Rail* rail) {
    rail_ = rail;
    selectedIndex_ = -1;
    auto& points = rail_->GetControlPoints();

    // ==========================================
    // レールの保存データを読み込む
    // ==========================================
    GlobalVariables* gv = GlobalVariables::GetInstance();
    const char* groupName = "RailData";
    gv->CreateGroup(groupName);

    gv->AddItem(groupName, "PointCount", -1);
    int count = gv->GetIntValue(groupName, "PointCount");

    if (count != -1) {
        // レールを上書きする
        points.clear();
        for (int i = 0; i < count; ++i) {
            char key[32];
            sprintf_s(key, "Point_%d", i);
            gv->AddItem(groupName, key, Vector3(0, 0, 0)); // エラー回避用
            points.push_back(gv->GetVector3Value(groupName, key));
        }
    }
    else {
        gv->SetValue(groupName, "PointCount", static_cast<int32_t>(points.size()));
        for (size_t i = 0; i < points.size(); ++i) {
            char key[32];
            sprintf_s(key, "Point_%d", static_cast<int>(i));
            gv->AddItem(groupName, key, points[i]);
        }
        gv->SaveFile(groupName);
    }
}

void RailEditor::Update(DebugCamera* dCamera, RailCamera* rCamera) {
    if (!rail_ || !dCamera || !rCamera) return;

    auto& points = rail_->GetControlPoints();

    // =================================================
    //  エディター専用操作パネル
    // =================================================
#ifdef USE_IMGUI
    ImGui::Begin("Rail Editor");

    ImGui::Text("--- Camera Control ---");
    bool* isPlay = rCamera->GetIsPlayPtr();
    if (ImGui::Button(*isPlay ? "Pause ||" : "Play >")) {
        *isPlay = !(*isPlay);
    }
    ImGui::SameLine();
    if (ImGui::Button("Rewind |<")) {
        *isPlay = false;
        rCamera->SetT(0.0f);
    }

    float t = rCamera->GetT();
    if (ImGui::SliderFloat("Timeline", &t, 0.0f, 1.0f)) {
        rCamera->SetT(t);
        *isPlay = false;
    }
    ImGui::SliderFloat("Speed", rCamera->GetSpeedPtr(), 0.0001f, 0.01f, "%.4f");

    ImGui::Separator();
    ImGui::Text("--- Points Control ---");
    ImGui::Text("Total Points: %d", (int)points.size());

    // ==========================================
    // 常に表示される「一番奥」への追加・削除ボタン
    // ==========================================
    if (ImGui::Button("Add Point (End)")) {
        Vector3 newPos = points.back();
        newPos.z += 50.0f; // さらに奥へ追加
        points.push_back(newPos);
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove Point (End)") && points.size() > 4) {
        points.pop_back();
        // もし消した点を選択中だったら選択解除する
        if (selectedIndex_ >= (int)points.size()) { selectedIndex_ = -1; }
    }

    ImGui::Spacing();
    if (ImGui::Button("SAVE RAIL DATA", ImVec2(200, 30))) {
        GlobalVariables* gv = GlobalVariables::GetInstance();
        const char* groupName = "RailData";

        gv->SetValue(groupName, "PointCount", static_cast<int32_t>(points.size()));

        for (size_t i = 0; i < points.size(); ++i) {
            char key[32];
            sprintf_s(key, "Point_%d", static_cast<int>(i));
            gv->SetValue(groupName, key, points[i]);
        }

        gv->SaveFile(groupName);
    }
    ImGui::Spacing();
    ImGui::Separator();

    // ==========================================
    //  画面の円をクリックした時だけ表示される精密操作メニュー
    // ==========================================
    if (selectedIndex_ >= 0 && selectedIndex_ < (int)points.size()) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Selected: Point [%d]", selectedIndex_);

        // XYZのスライダーで数値を直接いじれる！
        ImGui::DragFloat3("Position", &points[selectedIndex_].x, 0.5f);

        ImGui::Spacing();

        // 選択した点の次に新しい点を追加する
        if (ImGui::Button("Insert Next Point")) {
            Vector3 newPos = points[selectedIndex_];
            newPos.z += 20.0f; // 少し奥にずらす
            points.insert(points.begin() + selectedIndex_ + 1, newPos);
            selectedIndex_++; // 選択を新しい点に自動で移す
        }
        ImGui::SameLine();

        // 選択した点だけをピンポイントで消す
        if (ImGui::Button("Delete This") && points.size() > 4) {
            points.erase(points.begin() + selectedIndex_);
            selectedIndex_ = -1; // 選択解除
        }
    }
    else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Click a green circle to edit specific point.");
    }

    ImGui::End();

    // =================================================
    // 3D座標を2Dに変換して画面に円を描画する
    // =================================================
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    Matrix4x4 matVP = Multiply(dCamera->GetViewMatrix(), dCamera->GetProjectionMatrix());

    bool isMouseClicked = ImGui::IsMouseClicked(0) && !ImGui::GetIO().WantCaptureMouse;
    float mouseX = ImGui::GetMousePos().x;
    float mouseY = ImGui::GetMousePos().y;

    for (size_t i = 0; i < points.size(); ++i) {
        Vector3 p = points[i];

        float w = p.x * matVP.m[0][3] + p.y * matVP.m[1][3] + p.z * matVP.m[2][3] + matVP.m[3][3];

        if (w > 0.1f) {
            float nx = (p.x * matVP.m[0][0] + p.y * matVP.m[1][0] + p.z * matVP.m[2][0] + matVP.m[3][0]) / w;
            float ny = (p.x * matVP.m[0][1] + p.y * matVP.m[1][1] + p.z * matVP.m[2][1] + matVP.m[3][1]) / w;

            float sx = (nx + 1.0f) * 0.5f * 1280.0f;
            float sy = (1.0f - ny) * 0.5f * 720.0f;

            ImU32 color = (i == selectedIndex_) ? IM_COL32(255, 50, 50, 255) : IM_COL32(50, 255, 50, 200);

            drawList->AddCircleFilled(ImVec2(sx, sy), 15.0f, color);

            char label[16];
            sprintf_s(label, "%d", (int)i);
            drawList->AddText(ImVec2(sx - 4, sy - 8), IM_COL32(255, 255, 255, 255), label);

            if (isMouseClicked) {
                float dist = std::sqrt((mouseX - sx) * (mouseX - sx) + (mouseY - sy) * (mouseY - sy));
                if (dist < 15.0f) {
                    selectedIndex_ = (int)i;
                }
            }
        }
    }
#endif
}