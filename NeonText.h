#pragma once
#include "NeonSign.h"
#include "DirectXCommon.h"
#include "Matrix4x4.h"
#include <vector>
#include <string>

class NeonText {
public:
	// コンストラクタとデストラクタ
	NeonText() = default;
	~NeonText();

	// 初期化と更新・描画
	void Initialize(DirectXCommon* dxCommon);
	void Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);
	void Draw();

	// 文字列を構築する関数
	void Print(const std::string& text, float startX, float startY, float scale);

	// 全ての文字の質感を一括で変更する関数
	void SetMaterial(float radius, float softness, float intensity, float r, float g, float b, float lengthOffset);

	// 今ある文字を消去する
	void Clear();

private:
	// 1文字を作る工場
	void CreateLetter(char c, float baseX, float baseY, float scale);

	DirectXCommon* dxCommon_ = nullptr;

	// 生成されたネオンの棒を全て管理するリスト
	std::vector<NeonSign*> neonSigns_;

	// マテリアルの設定値を保持しておく変数
	float radius_ = 0.03f;
	float softness_ = 15.0f;
	float intensity_ = 8.0f;
	float color_[3] = { 0.0f, 0.8f, 1.0f };
	float lengthOffset_ = -0.2f;

	float time_ = 0.0f;
};