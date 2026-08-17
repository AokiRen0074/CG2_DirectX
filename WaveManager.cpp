#include "WaveManager.h"
#include "BaseEnemy.h" 
#include "GlobalValiables.h"
#include "WeakEnemyCross.h"
#include "WeakEnemySpinCore.h"
#include "WeakEnemyTriangle.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void WaveManager::Initialize() {
	currentWave_ = 0;
	isWaveActive_ = false;
	waveIntervalTimer_ = 120;

	// --- 【仮の設計図】 ---
	waveDatas_[0].enemyCount = 3;
	waveDatas_[0].enemies[0].position = { -15.0f, 0.0f, 150.0f };
	waveDatas_[0].enemies[0].type = 1;
	waveDatas_[0].enemies[1].position = { 0.0f, 0.0f, 150.0f };
	waveDatas_[0].enemies[1].type = 1;
	waveDatas_[0].enemies[2].position = { 15.0f, 0.0f, 150.0f };
	waveDatas_[0].enemies[2].type = 1;

	waveDatas_[1].enemyCount = 5;
	waveDatas_[1].enemies[0].position = { 0.0f, 0.0f, 150.0f };
	waveDatas_[1].enemies[1].position = { -10.0f, 0.0f, 160.0f };
	waveDatas_[1].enemies[2].position = { 10.0f, 0.0f, 160.0f };
	waveDatas_[1].enemies[3].position = { -20.0f, 0.0f, 170.0f };
	waveDatas_[1].enemies[4].position = { 20.0f, 0.0f, 170.0f };
	for (int i = 0; i < 5; ++i) waveDatas_[1].enemies[i].type = 1;

	LoadData();
}

void WaveManager::Update(std::list<BaseEnemy*>& enemies, Player* player, GameScene* gameScene) {
	
	/*----------------
	ほっとリロード
	---------------------*/
	if (isReloadRequested_) {
		// 今画面にいる敵をすべてメモリから消去する
		for (BaseEnemy* enemy : enemies) {
			delete enemy;
		}
		enemies.clear();

		StartWave(editWaveIndex_);

		// フラグを下ろす
		isReloadRequested_ = false;
	}

	// UIで書き換えた座標を適応する
	if (isDataModifiedThisFrame_) {
		// 編集中のウェーブが、今まさに画面でプレイ中のウェーブと同じなら同期
		if (editWaveIndex_ == currentWave_) {
			for (BaseEnemy* enemy : enemies) {
				int idx = enemy->GetSpawnIndex();
				// 念のため、配列外アクセスを防ぐ安全確認
				if (idx >= 0 && idx < waveDatas_[editWaveIndex_].enemyCount) {
					enemy->SetPosition(waveDatas_[editWaveIndex_].enemies[idx].position);
				}
			}
		}
		isDataModifiedThisFrame_ = false; 
	}
	
	if (!isWaveActive_) {
		if (--waveIntervalTimer_ <= 0) {
			StartWave(currentWave_ + 1);
		}
	}
	else {
		if (enemiesSpawned_ < enemiesToSpawn_) {
			if (--spawnTimer_ <= 0) {
				SpawnEnemy(currentWave_, enemiesSpawned_, enemies, player, gameScene);
				enemiesSpawned_++;
				spawnTimer_ = 10;
			}
		}
		else if (enemies.empty()) {
			isWaveActive_ = false;
			waveIntervalTimer_ = 180;
		}
	}
}

void WaveManager::StartWave(int wave) {
	if (wave >= kMaxWaves) return;
	currentWave_ = wave;
	isWaveActive_ = true;
	enemiesSpawned_ = 0;
	enemiesToSpawn_ = waveDatas_[currentWave_].enemyCount;
	spawnTimer_ = 60;
}

void WaveManager::SpawnEnemy(int wave, int enemyIndex, std::list<BaseEnemy*>& enemies, Player* player, GameScene* gameScene) {
	if (wave < 0 || wave >= kMaxWaves) return;
	if (enemyIndex < 0 || enemyIndex >= waveDatas_[wave].enemyCount) return;

	EnemySpawnData& data = waveDatas_[wave].enemies[enemyIndex];
	BaseEnemy* newEnemy = nullptr;

	// 将来的な敵種類の分岐
	if (data.type == 1) {
		newEnemy = new BaseEnemy();
	}
	else if (data.type == 2) {
		// クロス敵
		newEnemy = new WeakEnemyCross();
	}

	else if (data.type == 3) {
		newEnemy = new WeakEnemySpinCore();
	}
	
	else if (data.type == 4) {
		newEnemy = new WeakEnemyTriangle();
	}
	else {
		newEnemy = new BaseEnemy();
	}

	newEnemy->Initialize(player);
	newEnemy->SetGameScene(gameScene);
	newEnemy->SetPosition(data.position);

	newEnemy->SetSpawnIndex(enemyIndex);

	enemies.push_back(newEnemy);
}

void WaveManager::DrawImGui() {
#ifdef USE_IMGUI
	if (ImGui::TreeNodeEx("Wave Editor", ImGuiTreeNodeFlags_DefaultOpen)) {

		ImGui::Text("Current Playing Wave : %d", currentWave_ + 1);
		ImGui::Text("Enemies Spawned : %d / %d", enemiesSpawned_, enemiesToSpawn_);

		ImGui::Separator();

		// セーブボタン
		if (ImGui::Button("SAVE WAVE DATA (JSON)")) {
			SaveData();
		}

		ImGui::SameLine(); // ボタンを横に並べる

		// 編集中のウェーブを即座に再スタートする
		if (ImGui::Button("RELOAD & TEST THIS WAVE")) {
			isReloadRequested_ = true;
		}

		ImGui::Separator();

		ImGui::Separator();

		ImGui::SliderInt("Edit Wave", &editWaveIndex_, 0, kMaxWaves - 1);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			isReloadRequested_ = true;
		}

		WaveData& currentEditWave = waveDatas_[editWaveIndex_];

		ImGui::SliderInt("Enemy Count", &currentEditWave.enemyCount, 0, 30);
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			isReloadRequested_ = true;
		}

		ImGui::BeginChild("EnemyListRegion", ImVec2(0, 250), true);

		for (int i = 0; i < currentEditWave.enemyCount; ++i) {
			ImGui::PushID(i);
			if (ImGui::TreeNode((std::string("Enemy ") + std::to_string(i)).c_str())) {

				ImGui::InputInt("Type (1=Normal)", &currentEditWave.enemies[i].type);
				// Typeも入力が確定した時だけリロード
				if (ImGui::IsItemDeactivatedAfterEdit()) {
					isReloadRequested_ = true;
				}

				// （位置の変更はドラッグ中にリアルタイム反映させたいのでそのまま）
				if (ImGui::DragFloat3("Spawn Pos", &currentEditWave.enemies[i].position.x, 0.5f)) {
					isDataModifiedThisFrame_ = true;
				}

				ImGui::TreePop();
			}
			ImGui::PopID();
		
		}
		ImGui::EndChild();
		ImGui::TreePop();
	}
#endif
}

/*---------------------------
データのロード
-------------------------*/
void WaveManager::LoadData() {
	GlobalVariables* global = GlobalVariables::GetInstance();
	const std::string groupName = "WaveData";

	// グループを追加
	global->CreateGroup(groupName);

	for (int w = 0; w < kMaxWaves; ++w) {
		std::string wavePrefix = "Wave_" + std::to_string(w);

		// 敵の総数を登録・取得
		global->AddItem(groupName, wavePrefix + "_Count", waveDatas_[w].enemyCount);
		waveDatas_[w].enemyCount = global->GetIntValue(groupName, wavePrefix + "_Count");

		for (int e = 0; e < 30; ++e) { // 最大30体
			std::string enemyPrefix = wavePrefix + "_Enemy_" + std::to_string(e);

			// タイプと座標を登録・取得
			global->AddItem(groupName, enemyPrefix + "_Type", waveDatas_[w].enemies[e].type);
			global->AddItem(groupName, enemyPrefix + "_Pos", waveDatas_[w].enemies[e].position);

			waveDatas_[w].enemies[e].type = global->GetIntValue(groupName, enemyPrefix + "_Type");
			waveDatas_[w].enemies[e].position = global->GetVector3Value(groupName, enemyPrefix + "_Pos");
		}
	}
}

/*-----------------------------------
データのセーブ
---------------------------------*/
void WaveManager::SaveData() {
	GlobalVariables* global = GlobalVariables::GetInstance();
	const std::string groupName = "WaveData";

	for (int w = 0; w < kMaxWaves; ++w) {
		std::string wavePrefix = "Wave_" + std::to_string(w);

		// 変更された数値をセット
		global->SetValue(groupName, wavePrefix + "_Count", waveDatas_[w].enemyCount);

		for (int e = 0; e < 30; ++e) {
			std::string enemyPrefix = wavePrefix + "_Enemy_" + std::to_string(e);

			global->SetValue(groupName, enemyPrefix + "_Type", waveDatas_[w].enemies[e].type);
			global->SetValue(groupName, enemyPrefix + "_Pos", waveDatas_[w].enemies[e].position);
		}
	}

	global->SaveFile(groupName);
}