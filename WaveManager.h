#pragma once
#include "Vector3.h"
#include <list>
#include <string>
#include <EnemyBullet.h>

class BaseEnemy;
class Player;
class GameScene;

// 1体分の出現データ
struct EnemySpawnData {
	int type;
	Vector3 position;

	int moveState;     
	Vector3 direction; 
	float speed;     

	Vector3 scale = { 15.0f, 1.0f, 1.0f };
	Vector3 rotation = { 0.0f, 0.0f, 0.0f };
};

// 1ウェーブ分のデータ
struct WaveData {
	int enemyCount = 0;
	EnemySpawnData enemies[30];
};

class WaveManager {
public:
	// 初期化
	void Initialize();

	// ウェーブの更新とスポーン処理
	void Update(std::list<BaseEnemy*>& enemies, Player* player, GameScene* gameScene);

	// ImGuiの描画処理
	void DrawImGui();

	// jsonデータの保存っと読み込み
	void SaveData();
	void LoadData();
	bool IsWaveActive() const { return isWaveActive_; }
	int GetCurrentWave() const { return currentWave_ + 1; }

	// ウェーブをやり直す関数
	void RestartCurrentWave(std::list<BaseEnemy*>& enemies, std::list<EnemyBullet*>& enemyBullets);

private:
	void StartWave(int wave);
	void SpawnEnemy(int wave, int enemyIndex, std::list<BaseEnemy*>& enemies, Player* player, GameScene* gameScene);

	static const int kMaxWaves = 10;
	WaveData waveDatas_[kMaxWaves];


	int spawnIndex_ = -1;

	bool isDataModifiedThisFrame_ = false;

	// リロードフラグ
	bool isReloadRequested_ = false;

	int editWaveIndex_ = 0;
	int currentWave_ = 0;
	int spawnTimer_ = 0;
	int enemiesToSpawn_ = 0;
	int enemiesSpawned_ = 0;
	bool isWaveActive_ = false;
	int waveIntervalTimer_ = 120;


};