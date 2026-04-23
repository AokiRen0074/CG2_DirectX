#include "Logger.h"
#include "WindowApp.h"
#include "DirectXCommon.h"
#include <format>
#include <dxgidebug.h>
#include "Object3d.h"

#pragma comment(lib,"dxguid.lib")

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	// 各機能のインスタンス
	WindowApp* winApp = WindowApp::GetInstance();
	DirectXCommon* dxCommon = new DirectXCommon();
	Object3d* object3d = new Object3d();

	// 初期化処理
	Logger::Initialize();
	Logger::Log("Hello DirectX!\n");

	// std::formatを使ったログ
	Logger::Log(std::format("window size:{} x {}\n", WindowApp::kClientWidth, WindowApp::kClientHeight));

	// ConvertString を使ったログの利用例
	std::wstring testWString = L"test\n";
	Logger::Log(Logger::ConvertString(testWString));

	winApp->Initialize();
	dxCommon->Initialize(winApp);
	object3d->Initialize(dxCommon);
	
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
		object3d->Draw();


		// 描画の終了
		dxCommon->PostDraw();

		/*-----------------------------
		描画処理はここまで
		----------------------------*/
	}

	// 終了処理
	// 各オブジェクトはComPtrを使用しているので、dxCommonをdeleteした際に生成と逆順でReleaseされる
	delete dxCommon;
	delete object3d;
	winApp->Finalize();
	Logger::Finalize();

#ifdef _DEBUG
	// リソースリークチェック
	IDXGIDebug1* debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);

		debug->Release();
	}
#endif

	return 0;
}