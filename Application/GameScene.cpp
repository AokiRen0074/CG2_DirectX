#include "GameScene.h"
#include "Input/Input.h"
#include "2D/TextureManager.h"
#include "Application/Character/Player.h"
#include "AxisIndicator.h"
#include "GlobalValiables.h"
#include "WindowApp.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif


// ==========================================
// 指定された文字列を、自動で横に並べて配置する関数
// ==========================================
void GameScene::PrintNeon(const std::string& text, float startX, float startY, float scale) {
	float currentX = startX;

	// ★変更：文字の間隔も、指定されたサイズ（scale）に合わせて縮小・拡大する！
	float letterSpacing = 2.0f * scale;

	for (char c : text) {
		if (c == ' ') {
			currentX += letterSpacing;
			continue;
		}

		// ★変更：工場にもサイズ（scale）を伝える
		CreateLetter(c, currentX, startY, scale);

		currentX += letterSpacing;
	}
}


// ==========================================
// 1文字ごとの「棒の組み合わせ」を定義する工場
// ==========================================
void GameScene::CreateLetter(char c, float baseX, float baseY, float scale) {

	// ==========================================
	// 💡 魔法のラムダ式を改造
	// ==========================================
	auto addBar = [&](float ox, float oy, float len, float rot) {
		NeonSign* bar = new NeonSign();
		bar->Initialize(dxCommon_);

		// 🌟 変更：キャンバス（ポリゴン）は光が切れないように長めに用意する（ここは +0.5f のまま）
		float canvasLength = len + 0.5f;
		bar->SetTransform({ baseX + (ox * scale), baseY + (oy * scale), 0.0f }, { 1.0f * scale, canvasLength * scale, 1.0f }, rot);

		// 🌟 変更：光の芯は「設計図の長さ(len)」をそのまま使う！引き算しない！
		bar->SetTubeLength(len * scale);

		neonSigns_.push_back(bar);
		};

	// ==========================================
	// 💡 よく使う定型パーツ（デジタル時計のようなセグメント）
	// ==========================================
	auto vl = [&]() { addBar(-0.75f, 0.0f, 2.0f, 0.0f); };   // 左の縦棒（全体）
	auto vr = [&]() { addBar(0.75f, 0.0f, 2.0f, 0.0f); };    // 右の縦棒（全体）
	auto vm = [&]() { addBar(0.0f, 0.0f, 2.0f, 0.0f); };     // 中央の縦棒（全体）
	auto ht = [&]() { addBar(0.0f, 1.0f, 1.5f, 1.57f); };    // 上の横棒
	auto hm = [&]() { addBar(0.0f, 0.0f, 1.5f, 1.57f); };    // 真ん中の横棒
	auto hb = [&]() { addBar(0.0f, -1.0f, 1.5f, 1.57f); };   // 下の横棒
	auto vtl = [&]() { addBar(-0.75f, 0.5f, 1.0f, 0.0f); };  // 左上の短い縦棒
	auto vbl = [&]() { addBar(-0.75f, -0.5f, 1.0f, 0.0f); }; // 左下の短い縦棒
	auto vtr = [&]() { addBar(0.75f, 0.5f, 1.0f, 0.0f); };   // 右上の短い縦棒
	auto vbr = [&]() { addBar(0.75f, -0.5f, 1.0f, 0.0f); };  // 右下の短い縦棒

	// 小文字が入力されても、大文字として処理するように変換
	c = (char)std::toupper(c);

	// ==========================================
	// 💡 A〜Z の設計図（パーツを組み合わせるだけ！）
	// ==========================================
	switch (c) {
	case 'A':
		vl();
		vr();
		ht();
		hm();
		break;
	case 'B':
		vl();
		ht();
		hm();
		hb();
		vtr();
		vbr();
		break; // カクカクのB
	case 'C':
		vl();
		ht();
		hb();
		break;
	case 'D':
		vl();
		vr();
		ht();
		hb();
		break; // Oと同じ（ブロック体）
	case 'E':
		vl();
		ht();
		hm();
		hb();
		break;
	case 'F':
		vl();
		ht();
		hm();
		break;
	case 'G':
		vl();
		ht();
		hb();
		vbr();
		addBar(0.375f, 0.0f, 0.75f, 1.57f);
		break; // Gの右下の折り返し
	case 'H':
		vl();
		vr();
		hm();
		break;
	case 'I':
		vm();
		ht();
		hb();
		break; // 上下にヒゲがあるI
	case 'J':
		vr();
		hb();
		vbl();
		break;
	case 'K':
		vl();
		addBar(0.0f, 0.5f, 1.8f, -0.98f);
		addBar(0.0f, -0.5f, 1.8f, 0.98f);
		break; // 斜め線
	case 'L':
		vl();
		hb();
		break;
	case 'M':
		vl();
		vr();
		addBar(-0.375f, 0.5f, 1.25f, 0.64f);
		addBar(0.375f, 0.5f, 1.25f, -0.64f);
		break;
	case 'N':
		vl();
		vr();
		addBar(0.0f, 0.0f, 2.5f, 0.64f);
		break; // 斜め線(N)
	case 'O':
		vl();
		vr();
		ht();
		hb();
		break;
	case 'P':
		vl();
		vtr();
		ht();
		hm();
		break;
	case 'Q':
		vl();
		vr();
		ht();
		hb();
		addBar(0.4f, -0.6f, 1.2f, -0.78f);
		break; // Oに右下のヒゲ
	case 'R':
		vl();
		vtr();
		ht();
		hm();
		addBar(0.375f, -0.5f, 1.25f, 0.64f);
		break;
	case 'S':
		ht();
		hm();
		hb();
		vtl();
		vbr();
		break;
	case 'T':
		ht();
		vm();
		break;
	case 'U':
		vl();
		vr();
		hb();
		break;
	case 'V':
		addBar(-0.375f, 0.0f, 2.13f, 0.36f);
		addBar(0.375f, 0.0f, 2.13f, -0.36f);
		break;
	case 'W':
		vl();
		vr();
		addBar(-0.375f, -0.5f, 1.25f, -0.64f);
		addBar(0.375f, -0.5f, 1.25f, 0.64f);
		break;
	case 'X':
		addBar(0.0f, 0.0f, 2.5f, 0.64f);
		addBar(0.0f, 0.0f, 2.5f, -0.64f);
		break; // クロス
	case 'Y':
		addBar(-0.375f, 0.5f, 1.25f, 0.64f);
		addBar(0.375f, 0.5f, 1.25f, -0.64f);
		addBar(0.0f, -0.5f, 1.0f, 0.0f);
		break;
	case 'Z':
		ht();
		hb();
		addBar(0.0f, 0.0f, 2.5f, -0.64f);
		break; // 斜め線(Z)
	}
}

GameScene::~GameScene() {
	delete debugCamera_;
	delete player_;
	delete enemy_;
	//delete bulletModel_;
}

void GameScene::Initialize(DirectXCommon* dxCommon) {

	dxCommon_ = dxCommon;
	bloom_ = new Bloom();

	// 画面サイズ（1280x720）を渡す
	bloom_->Initialize(dxCommon_, WindowApp::kClientWidth, WindowApp::kClientHeight);


	/*-------------------------------
	ワールドトランスフォーム
	----------------------------------*/
	WorldTransform::SetDevice(dxCommon->GetDevice());


	// カメラの生成と初期化
	debugCamera_ = new DebugCamera();
	debugCamera_->Initialize();

	/*----------------------------
	ビュープロジェクションの初期化
	---------------------------------*/
	viewProjection_.Initialize();
	viewProjection_.translation_.z = -20.0f;
	viewProjection_.UpdateMatrix();


	/*-------------------------------
	3Dオブジェクトの生成と初期化
	----------------------------------*/

	// プレイヤー
	object3d_ = new Object3d();
	Object3d::StaticInitialize(dxCommon);
	object3d_->Initialize("Resources", "player.obj");

	textureHandle_ = TextureManager::Load("Resources/uvChecker.png");

	// エネミー
	enemyObject_ = new Object3d();
	enemyObject_->Initialize("Resources", "block.obj");
	enemyTex_ = TextureManager::Load("Resources/monsterBall.png");



	/*----------------------
	スプライトの生成と初期化
	-------------------------*/


	/*-------------------------------
	自キャラ生成と初期化
	----------------------------------*/
	Player::RegisterGlobalVariables();
	// 自キャラの生成
	player_ = new Player();

	// 自キャラの初期化
	player_->Initialize(object3d_, textureHandle_);


	// 敵キャラの生成
	enemy_ = new Enemy();

	// 敵キャラの生成
	enemy_->Initialize(enemyObject_, enemyTex_);

	// 敵キャラに自キャラのアドレスを渡す
	enemy_->SetPlayer(player_);

	/*-------------------------
	弾
	------------------------------*/

	PrintNeon("NEON", 0.0f, 0.0f, 0.7f);


	/*-----------------------
	軸表示
	------------------------*/
	AxisIndicator::GetInstance()->Initialize();

	// 軸方向の表示を有効にする
	AxisIndicator::GetInstance()->SetVisible(true);

	// 軸方向表示が参照するビュープロジェクションの指定
	AxisIndicator::GetInstance()->SetTargetCamera(&viewProjection_);

}

void GameScene::Update() {

		static float neonRadius = 0.03f;
		static float neonSoftness = 15.0f;
		static float neonIntensity = 8.0f;
		static float neonColor[3] = { 0.0f, 0.8f, 1.0f };
		static float neonLengthOffset = -0.2f;

#ifdef USE_IMGUI
		ImGui::Begin("Neon Control Panel");
		ImGui::SliderFloat("Radius (太さ)", &neonRadius, 0.001f, 0.1f);
		ImGui::SliderFloat("Length Offset (長さ微調整)", &neonLengthOffset, -1.0f, 1.0f);
		ImGui::SliderFloat("Softness (ぼかし)", &neonSoftness, 0.1f, 50.0f);
		ImGui::SliderFloat("Intensity (光の強さ)", &neonIntensity, 0.1f, 20.0f);
		ImGui::ColorEdit3("Color (色)", neonColor);
		ImGui::End();
#endif

	/*-------------------------
	デバッグカメラ
	--------------------------*/
#ifdef _DEBUG 
	if (Input::GetInstance()->TriggerKey(DIK_P)) {
		isDebugCameraActive_ = !isDebugCameraActive_;
	}


	if (isDebugCameraActive_) {
		debugCamera_->Update();

		viewProjection_.matView = debugCamera_->GetViewMatrix();
		viewProjection_.matProjection = debugCamera_->GetProjectionMatrix();
	}
	else {
		viewProjection_.UpdateMatrix();
	}
#endif


	/*------------------
	自キャラ更新
	----------------------*/

	player_->Update();



	/*------------------
	敵キャラ更新
	------------------*/
	if (enemy_) {
		enemy_->Update();
	}


	// オブジェクトの更新
	if (isDebugCameraActive_ && debugCamera_ != nullptr) {
		debugCamera_->Update();

		// ここで安全に取得する
		object3d_->SetCameraMatrix(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
	}



	for (NeonSign* sign : neonSigns_) {

		sign->SetMaterial(neonRadius, neonSoftness, neonIntensity, neonColor[0], neonColor[1], neonColor[2], neonLengthOffset);
		if (isDebugCameraActive_ && debugCamera_ != nullptr) {
			sign->UpdateCamera(debugCamera_->GetViewMatrix(), debugCamera_->GetProjectionMatrix());
		}
		else {
			sign->UpdateCamera(viewProjection_.matView, viewProjection_.matProjection);
		}
	}

	AxisIndicator::GetInstance()->Update();


#ifdef USE_IMGUI
	ImGui::ShowDemoWindow();
	GlobalVariables::GetInstance()->Update();
#endif
}

void GameScene::Draw() {

	// ==========================================
		// 1. 通常の描画（普通のモニター R8G8B8A8 に描くもの）
		// ==========================================
	AxisIndicator::GetInstance()->Draw();
	if (enemy_) {
		enemy_->Draw(viewProjection_);
	}
	player_->Draw(viewProjection_); // 普通の3Dプレイヤー


	// ==========================================
	// 2. ネオンの描画（ここからHDRキャンバス R16G16B16A16 に切り替え！）
	// ==========================================
	bloom_->PreDraw(); // キャンバスを切り替え

	// ★ ここでネオン系のオブジェクトを描画する！
	for (NeonSign* sign : neonSigns_) {
		sign->Draw();
	}
	// もしネオン自機（neonPlayer_）などがいるなら、それもここでDrawする

	bloom_->PostDraw(); // HDRキャンバスへの書き込み終了


	// ==========================================
	// 3. 仕上げの魔法（ぼかして光を溢れさせ、モニターに合成する）
	// ==========================================
	bloom_->Execute();    // コンピュートシェーダーでぼかし計算
	bloom_->DrawResult(); // モニターに最終結果をドン！と描画
}




