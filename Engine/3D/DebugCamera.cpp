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
	// マウス入力による角度・距離の更新
	// =================================================
	static float distance = 50.0f;

	if (!isImGuiHovered) {
		// 左ドラッグでカメラの角度
		if (input->PushMouseLeft()) {
			rotY_ += input->GetMouseMoveX() * 0.005f; // 左右に回る
			rotX_ += input->GetMouseMoveY() * 0.005f; // 上下に回る

			// 真上・真下に行き過ぎて画面がひっくり返るのを防止
			if (rotX_ > 1.5f) { rotX_ = 1.5f; }
			if (rotX_ < -1.5f) { rotX_ = -1.5f; }
		}

		// ホイールでオブジェクトにズームイン・ズームアウト
		float wheel = input->GetWheel();
		if (wheel != 0.0f) {
			distance -= wheel * 0.05f; // 感度調整
			if (distance < 1.0f) { distance = 1.0f; }
		}
	}

	// =================================================
	//  カメラ行列の計算
	// =================================================
	// まずは純粋な回転行列を作る
	Matrix4x4 rotXMat = MakeRotateXMatrix(rotX_);
	Matrix4x4 rotYMat = MakeRotateYMatrix(rotY_);
	matRot_ = Multiply(rotXMat, rotYMat);

	// 回転行列から、カメラのZ軸のベクトルを取り出す
	Vector3 forward = { matRot_.m[2][0], matRot_.m[2][1], matRot_.m[2][2] };



	translation_.x = -forward.x * distance;
	translation_.y = -forward.y * distance;
	translation_.z = -forward.z * distance;

	// ビュー行列の更新
	Matrix4x4 matTrans = MakeTranslateMatrix(translation_);
	Matrix4x4 matWorld = Multiply(matRot_, matTrans);
	matView_ = Inverse(matWorld);
}