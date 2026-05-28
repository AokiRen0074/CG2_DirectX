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
	bool isImGuiActiveKeyboard = false;

#ifdef USE_IMGUI
	// UIの上にマウスがあるか、UIを操作中か
	isImGuiHovered = ImGui::GetIO().WantCaptureMouse;
	// UIのテキストボックスなどに文字入力中か
	isImGuiActiveKeyboard = ImGui::GetIO().WantCaptureKeyboard;
#endif

	// =================================================
	// カメラの回転
	// =================================================
	float mouseMoveX = 0.0f;
	float mouseMoveY = 0.0f;
	float wheel = 0.0f;
	Vector3 move = { 0.0f, 0.0f, 0.0f };
	const float speed = 0.5f;


	if (!isImGuiHovered) {
		// 左ドラッグで回転
		if (input->PushMouseLeft()) {
			mouseMoveX = input->GetMouseMoveX() * 0.001f;
			mouseMoveY = input->GetMouseMoveY() * 0.001f;
		}

		// 右ドラッグで上下左右移動
		if (input->PushMouseRight()) {
			move.x = -input->GetMouseMoveX() * 0.05f;
			move.y = input->GetMouseMoveY() * 0.05f;
		}

		// ホイールで前後移動
		wheel = input->GetWheel();
		if (wheel != 0.0f) {
			move.z = wheel * 0.01f;
		}
	}


	if (!isImGuiActiveKeyboard) {
		if (input->PushKey(DIK_W)) { move.z += speed; }
		if (input->PushKey(DIK_S)) { move.z += -speed; }
		if (input->PushKey(DIK_D)) { move.x += speed; }
		if (input->PushKey(DIK_A)) { move.x += -speed; }
	}

	// =================================================
	// カメラ行列の計算と適用
	// =================================================
	// 追加の回転を計算
	Matrix4x4 matRotDelta = MakeIdentity4x4();
	Matrix4x4 rotX = MakeRotateXMatrix(mouseMoveY);
	Matrix4x4 rotY = MakeRotateYMatrix(mouseMoveX);
	matRotDelta = Multiply(rotX, rotY);
	matRot_ = Multiply(matRotDelta, matRot_);

	// 移動ベクトルをカメラの向きに合わせて回転させる
	Vector3 right = { matRot_.m[0][0], matRot_.m[0][1], matRot_.m[0][2] };
	Vector3 up = { matRot_.m[1][0], matRot_.m[1][1], matRot_.m[1][2] };
	Vector3 forward = { matRot_.m[2][0], matRot_.m[2][1], matRot_.m[2][2] };

	translation_.x += (right.x * move.x) + (up.x * move.y) + (forward.x * move.z);
	translation_.y += (right.y * move.x) + (up.y * move.y) + (forward.y * move.z);
	translation_.z += (right.z * move.x) + (up.z * move.y) + (forward.z * move.z);

	// ビュー行列の更新
	Matrix4x4 matTrans = MakeTranslateMatrix(translation_);
	Matrix4x4 matWorld = Multiply(matRot_, matTrans);
	matView_ = Inverse(matWorld);

}