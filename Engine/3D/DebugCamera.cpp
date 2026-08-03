#include "DebugCamera.h"
#include "Input.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

// ==========================================
// 初期化
// ==========================================
void DebugCamera::Initialize() {
	// メンバ変数の初期化
	matRot_ = MakeIdentity4x4();
	translation_ = { 0.0f, 0.0f, -50.0f };


	matProjection_ = MakePerspectiveFovMatrix(0.45f, 1280.0f / 720.0f, 0.1f, 100.0f);
}

// ==========================================
// 更新
// ==========================================
void DebugCamera::Update() {
	Input* input = Input::GetInstance();
	if (!input) { return; }

	bool isImGuiHovered = false;
#ifdef USE_IMGUI
	isImGuiHovered = ImGui::GetIO().WantCaptureMouse;
#endif

	// =================================================
	//  マウス入力による角度の更新
	// =================================================
	if (!isImGuiHovered) {

		if (input->PushMouseRight()) {
			rotY_ += input->GetMouseMoveX() * 0.005f;
			rotX_ += input->GetMouseMoveY() * 0.005f;

			// 真上・真下に行き過ぎて画面がひっくり返るのを防止
			if (rotX_ > 1.5f) { rotX_ = 1.5f; }
			if (rotX_ < -1.5f) { rotX_ = -1.5f; }
		}
	}

	// =================================================
	// メラ行列の計算と、方向ベクトルの抽出
	// =================================================
	Matrix4x4 rotXMat = MakeRotateXMatrix(rotX_);
	Matrix4x4 rotYMat = MakeRotateYMatrix(rotY_);
	matRot_ = Multiply(rotXMat, rotYMat);

	// 回転行列ベクトルを取り出す
	Vector3 right = { matRot_.m[0][0], matRot_.m[0][1], matRot_.m[0][2] };
	Vector3 up = { matRot_.m[1][0], matRot_.m[1][1], matRot_.m[1][2] };
	Vector3 forward = { matRot_.m[2][0], matRot_.m[2][1], matRot_.m[2][2] };

	// =================================================
	//  キーボード入力による座標の移動 
	// =================================================
	if (!isImGuiHovered) {
		float moveSpeed = 0.5f; // カメラの移動スピード

		// Shiftキーを押している間はダッシュ
		if (input->PushKey(DIK_LSHIFT)) { moveSpeed = 2.0f; }

		// Vector3の演算子オーバーロードのおかげで直感的に書けます
		if (input->PushKey(DIK_W)) { translation_ = translation_ + forward * moveSpeed; }
		if (input->PushKey(DIK_S)) { translation_ = translation_ - forward * moveSpeed; }
		if (input->PushKey(DIK_D)) { translation_ = translation_ + right * moveSpeed; }
		if (input->PushKey(DIK_A)) { translation_ = translation_ - right * moveSpeed; }
		if (input->PushKey(DIK_E)) { translation_ = translation_ + up * moveSpeed; } // 上昇
		if (input->PushKey(DIK_Q)) { translation_ = translation_ - up * moveSpeed; } // 下降
	}

	// =================================================
	// ビュー行列の更新
	// =================================================
	Matrix4x4 matTrans = MakeTranslateMatrix(translation_);
	Matrix4x4 matWorld = Multiply(matRot_, matTrans);
	matView_ = Inverse(matWorld);
}