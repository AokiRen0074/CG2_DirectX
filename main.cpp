#include "Logger.h"
#include "WindowApp.h"
#include "DirectXCommon.h"

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	// 各機能のインスタンス
	WindowApp* winApp = WindowApp::GetInstance();
	DirectXCommon* dxCommon = new DirectXCommon();

	// 初期化処理
	Logger::Initialize();
	Logger::Log("Hello DirectX!\n");

	winApp->Initialize();
	dxCommon->Initialize(winApp);

	// メインループ
	while (true) {
		// メッセージ処理（×ボタンが押されたらループを抜ける）
		if (winApp->ProcessMessage()) {
			break;
		}

		/*------------------------
		更新処理はここから
		----------------------------*/




		/*--------------------
		更新処理はここまで
		-----------------------------*/

		/*------------------------------
		描画処理はここから
		-------------------------------*/
		// 描画の開始
		dxCommon->PreDraw();

		// モデルの描画など



		// 描画の終了
		dxCommon->PostDraw();

		/*-----------------------------
		描画処理はここまで
		----------------------------*/
	}

	// 終了処理
	delete dxCommon;
	winApp->Finalize();
	Logger::Finalize();

	return 0;
}