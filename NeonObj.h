#pragma once
#include "NeonModel.h"
#include "WorldTransform.h"
#include "ViewProjection.h"
#include "2D/TextureManager.h"
#include <string>

class NeonObj {
public:
	~NeonObj();

	// 初期化（モデルのファイル名を受け取る）
	void Initialize(const std::string& directoryPath, const std::string& filename);

	// 更新（フリッカーや行列の計算）
	void Update();

	// 描画
	void Draw(const ViewProjection& viewProjection);

	// ImGuiパネルを描画する関数
	void DrawImGui(const std::string& label);

private:
	NeonModel* model_ = nullptr;
	WorldTransform transform_;
	uint32_t dummyTexture_ = 0;

	// --- ネオンの質感パラメーター ---
	float color_[3] = { 1.0f, 0.0f, 0.0f }; // 初期色は赤
	float intensity_ = 8.0f;                // 光の強さ
	float radius_ = 0.03f;                  // 太さ
	float softness_ = 15.0f;                // ぼかし具合
	float lengthOffset_ = 0.0f;             // 管の長さ

	// --- フリッカー（チカチカ）用 ---
	bool isFlicker_ = true; // チカチカさせるかどうかのスイッチ
	float time_ = 0.0f;
};