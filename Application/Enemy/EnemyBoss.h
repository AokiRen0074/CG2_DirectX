#pragma once
#include "BaseEnemy.h"
#include <string>
#include "WarningUI.h"
#include "EnemyBullet.h"
#include "Audio/Audio.h"

class EnemyBoss : public BaseEnemy {
public:

	// 状態遷移
	enum class BossState {
		Intro,   // 登場演出中
		Phase1,  // 戦闘中
		Phase2,  // 発狂モード
		Dying    // 撃破演出
	};

	struct ChargePixel {
		Vector3 position;
		bool isActive;
	};

	struct ShockwaveRing {
		WorldTransform transform;
		bool isActive = false;
	};

	~EnemyBoss();
	void Initialize(Player* player) override;
	void Update() override;
	void DrawNeon(const ViewProjection& viewProjection) override;
	void OnCollision() override;
	void DrawImGui() override;

	bool IsDying() const { return currentState_ == BossState::Dying; }
	float GetAnimeTime() const { return animeTime_; }

	// ボスのHPバーUIに渡すゲッター
	int GetCurrentHp() const { return currentHp_; }
	int GetMaxHp() const { return maxHp_; }
	bool IsBattleStarted() const { return currentState_ != BossState::Intro; }

	

private:
	// ==========================================
	// モデルポインタ 
	// ==========================================
	BodyModel* armorModels_[2] = { nullptr };
	BodyModel* turretModels_[8] = { nullptr };

	NeonModel* coreModel_ = nullptr;
	NeonModel* haloModel_ = nullptr;
	NeonModel* wingModels_[6] = { nullptr };

	WorldTransform transformArmor_[2];
	WorldTransform transformTurrets_[8];
	WorldTransform transformCore_;
	WorldTransform transformHalo_;
	WorldTransform transformWings_[6];

	uint32_t whiteTexture_ = 0u;

	// ==========================================
	// 色と輝度の管理
	// ==========================================
	float armorColor_[3] = { 0.4f, 0.4f, 0.45f };
	float turretColor_[3] = { 0.3f, 0.3f, 0.35f };

	float coreColor_[3] = { 1.0f, 0.0f, 0.2f };
	float coreIntensity_ = 15.0f;

	float wingColor_[3] = { 0.0f, 0.8f, 1.0f };
	float wingIntensity_ = 8.0f;

	float haloColor_[3] = { 1.0f, 1.0f, 0.8f };
	float haloIntensity_ = 10.0f;

	// ステータスと演出用
	int maxHp_ = 500;
	int currentHp_ = 500;
	int hitTimer_ = 0;
	int flashTimer_ = 0;
	float animeTime_ = 0.0f;

	// ==========================================
	// 部位破壊フラグ！
	// ==========================================
	bool isTurretActive_[8];
	bool isArmorActive_[2];

	float attackTimer_ = 0.0f;
	int attackPhase_ = 0; // 0:タレット, 1:ミサイル, 2:レーザー

	int turretFireCount_ = 0;

	void AttackTurret();
	void AttackMissile();
	void AttackLaser();

	BossState currentState_ = BossState::Intro;
	float introTimer_ = 0.0f;     // 登場演出の時間を計る
	Vector3 startPos_;            // 登場開始の座標
	Vector3 targetPos_;

	bool isFirstFrame_ = true;

	EnemyBullet* standbyBullets_[8] = { nullptr };

	/*-----------------------------------
	レーザー
	--------------------------------*/
	NeonModel* laserModel_ = nullptr;
	WorldTransform transformLaser_;
	bool isLaserActive_ = false;

	// チャージ用の吸収エフェクト
	NeonModel* pixelModel_ = nullptr;
	static const int kMaxChargePixels = 60;
	ChargePixel chargePixels_[kMaxChargePixels];
	WorldTransform pixelTransforms_[kMaxChargePixels];

	// レーザー専用の当たり判定チェック関数
	bool CheckLaserCollision(const Vector3& targetPos);


	ShockwaveRing shockwaveRings_[8];
	void AttackRing();
	bool CheckRingCollision(const Vector3& targetPos, const ShockwaveRing& ring);

	SoundData chargeSound_;
	SoundData beamSound_;
	SoundData smallExplosionSound_;
	SoundData bigExplosionSound_;
	bool hasPlayedChargeSound_ = false;
	bool hasPlayedBeamSound_ = false;
	bool hasPlayedBigExplosion_ = false;
};