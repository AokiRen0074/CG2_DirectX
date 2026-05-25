#include "Logger.h"
#include "WindowApp.h"
#include "DirectXCommon.h"
#include <format>
#include <dxgidebug.h>
#include "Object3d.h"
#include "Sprite.h"
#include "Audio.h"


#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#endif

#pragma comment(lib,"dxguid.lib")

struct D3DResourceLeakChecker {
	~D3DResourceLeakChecker() {
		Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
			// メモリリークを判定
			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		}
	}
};

int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	D3DResourceLeakChecker leakCheck;


	CoInitializeEx(0,COINIT_MULTITHREADED);

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

	D3D12_GPU_DESCRIPTOR_HANDLE sharedTextureHandle = object3d->GetTextureSrvHandleGPU();
	Sprite* sprite = new Sprite();
	sprite->Initialize(dxCommon, sharedTextureHandle);

	// 音
	Audio* audio = new Audio();
	audio->Initialize();

	SoundData soundData = audio->SoundLoadWave("Resources/Alarm01.wav");
	
	audio->SoundPlayWave(soundData);



	// メインループ
	while (true) {
		// メッセージ処理
		if (winApp->ProcessMessage()) {
			break;
		}

		/*------------------------
		更新処理はここから
		----------------------------*/

		
#ifdef USE_IMGUI
		// imguiのフレーム開始
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// 怪異発揚UIの処理
		ImGui::ShowDemoWindow();

#endif

		object3d->Update();

		sprite->Update();

#ifdef USE_IMGUI
		ImGui::Render();
#endif
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
		//sprite->Draw();


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
	delete sprite;
	CoUninitialize();
	audio->SoundUnload(&soundData);
	audio->Finalize();
#ifdef USE_IMGUI
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
#endif
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