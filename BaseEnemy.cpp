#include "BaseEnemy.h"
#include "EnemyStateApproach.h"
#include <cassert>
#include "cmath"
#include "Application/Character/Player.h"
#include "CollisionConfig.h"
#include "GameScene.h"


#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

static uint32_t sDummyTexture = 0;
static uint32_t sTailTexture = 0;
static BodyModel* sModelBase = nullptr;
static NeonModel* sModelLines = nullptr;
static NeonModel* sModelTails[5] = { nullptr };
static NeonModel* sModelRing = nullptr;
static NeonModel* sBulletModel = nullptr;



// デストラクタ
BaseEnemy::~BaseEnemy() {


	for (TimedCall* timedCall : timedCalls_) {
		delete timedCall;
	}

}

void BaseEnemy::StaticInitialize() {
	if (sModelBase == nullptr) {
		sDummyTexture = TextureManager::Load("Resources/Enemy/PlayerTex.png");
		sTailTexture = TextureManager::Load("Resources/Enemy/PlayerTex.png");

		sModelBase = new BodyModel();
		sModelBase->Initialize("Resources/Enemy", "Enemy_Base.obj");

		sModelLines = new NeonModel();
		sModelLines->Initialize("Resources/Enemy", "enemy_lines.obj");

		for (int i = 0; i < 5; ++i) {
			sModelTails[i] = new NeonModel();
			sModelTails[i]->Initialize("Resources/Enemy", "enemy_tail.obj");
		}

		sModelRing = new NeonModel();
		sModelRing->Initialize("Resources/Enemy", "Enemy_ring.obj");

		sBulletModel = new NeonModel();
		sBulletModel->Initialize("Resources/Bullet", "EnemyBuillet.obj");
	}



}

void BaseEnemy::Initialize(Player* player) {

	assert(player);
	player_ = player;

	worldTransform_.Initialize();
	transformLines_.Initialize();

	for (int i = 0; i < 5; ++i) {
		transformTails_[i].Initialize();
	}

	dummyTexture_ = sDummyTexture;
	tailTexture_ = sTailTexture;
	modelBase_ = sModelBase;
	modelLines_ = sModelLines;
	for (int i = 0; i < 5; ++i) {
		modelTails_[i] = sModelTails[i];
	}
	modelRing_ = sModelRing;
	bulletModel_ = sBulletModel;

	// 初期座標
	worldTransform_.scale_ = { 1.0f, 1.0f, 1.0f };
	worldTransform_.rotation_ = { -1.5708f, 0.0f, 0.0f };
	worldTransform_.translation_ = { 5.0f, 0.0f, 50.0f };
	
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	// 最初の状態
	state_ = new EnemyStateApproach();
	state_->SetEnemy(this);


	// 自分の属性を敵に設定
	SetCollisionAttribute(kCollisionAttributeEnemy);
	//当たる相手を敵に設定
	SetCollisionMask(~kCollisionAttributeEnemy);

	ApproachPhaseInitialize();

}

// 接近フェーズ初期化
void BaseEnemy::ApproachPhaseInitialize() {
	// 最初の発射を予約
	FireAndReset();

}

// 発射してリセット
void BaseEnemy::FireAndReset() {

	// 弾を発射
	Fire();

	timedCalls_.push_back(
		new TimedCall(std::bind(&BaseEnemy::FireAndReset, this), kFireInterval)
	);

}


/*-------------------------------
攻撃
--------------------------------*/
void BaseEnemy::Fire() {

	assert(player_);

	// 弾の速さ
	const float kBulletSpeed = 1.0f;

	Vector3 playerPos = player_->GetWorldPosition();
	// 敵キャラ自身のワールド座標を取得する
	Vector3 enemyPos = GetWorldPosition();

	// 敵から自キャラへの差分ベクトルを求める
	Vector3 velocity;
	velocity.x = playerPos.x - enemyPos.x;
	velocity.y = playerPos.y - enemyPos.y;
	velocity.z = playerPos.z - enemyPos.z;

	// ベクトルの正規化

	float length = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
	if (length != 0.0f) {
		velocity.x /= length;
		velocity.y /= length;
		velocity.z /= length;
	}

	// ベクトルの長さを速さに合わせる
	velocity.x *= kBulletSpeed;
	velocity.y *= kBulletSpeed;
	velocity.z *= kBulletSpeed;

	EnemyBullet* newBullet = new EnemyBullet();
	newBullet->SetPlayer(player_);
	newBullet->Initialize(bulletModel_, enemyPos, velocity, dummyTexture_);

	// 弾を登録する
	if (gameScene_) {
		gameScene_->AddEnemyBullet(newBullet);
	}




}

/*----------------------------------------
衝突時コールバック
-----------------------------------*/
void BaseEnemy::OnCollision() {

	isDead_ = true;
}


/*--------------------------
更新処理
------------------------------------*/
void BaseEnemy::Update() {
	// 終了したイベントを削除
	timedCalls_.remove_if([](TimedCall* timedCall) {
		if (timedCall->isFinished()) {
			delete timedCall;
			return true;
		}
		return false;
		});


	for (TimedCall* timedCall : timedCalls_) {
		timedCall->Update();
	}



	// 状態遷移
	if (state_) {
		state_->Update();
	}




	// 時間の更新
	time_ += 1.0f / 60.0f;

	// 尻尾5個の座標更新
	for (int i = 0; i < 5; ++i) {
		transformTails_[i].scale_ = worldTransform_.scale_;
		transformTails_[i].rotation_ = worldTransform_.rotation_;

		// 順番にズラす
		float offset = i * 1.0f;
		transformTails_[i].translation_ = {
			worldTransform_.translation_.x,
			worldTransform_.translation_.y,
			worldTransform_.translation_.z + offset
		};

		transformTails_[i].matWorld_ = MakeAffineMatrix(transformTails_[i].scale_, transformTails_[i].rotation_, transformTails_[i].translation_);
		transformTails_[i].TransferMatrix();
	}

	// 行列の更新
	worldTransform_.matWorld_ = MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	worldTransform_.TransferMatrix();


	transformLines_.translation_ = worldTransform_.translation_;
	transformLines_.scale_ = worldTransform_.scale_;


	transformLines_.rotation_.x = -1.5708f;


	transformLines_.rotation_.z += 0.05f;
	// Lines専用の行列を更新
	transformLines_.matWorld_ = MakeAffineMatrix(transformLines_.scale_, transformLines_.rotation_, transformLines_.translation_);
	transformLines_.TransferMatrix();
}


// 描画処理

void BaseEnemy::Draw(const ViewProjection& viewProjection) {



}

void BaseEnemy::DrawNeon(const ViewProjection& viewProjection) {
	if (modelBase_) {
		modelBase_->SetColor(bodyColor_[0], bodyColor_[1], bodyColor_[2], 1.0f);
		Vector3 lightDir = { -1.0f, -1.0f, 1.0f };
		modelBase_->SetLight(1.0f, 1.0f, 1.0f, 1.0f, lightDir);
		modelBase_->Draw(worldTransform_, viewProjection, dummyTexture_);
	}

	if (modelLines_) {
		modelLines_->SetNeonColor(neonIntensity_, neonColor_[0], neonColor_[1], neonColor_[2]);
		modelLines_->Draw(transformLines_, viewProjection, dummyTexture_);
	}

	float* colors[3] = { tailColor1_, tailColor2_, tailColor3_ };
	float speed = 5.0f;
	float t = time_ * speed;

	for (int i = 0; i < 5; ++i) {
		// 安全対策：モデルが作られていなければスキップ
		if (modelTails_[i] == nullptr) continue;

		// 各矢印の位置を計算
		float x = t - i;
		int colorGroup = static_cast<int>(std::floor(x / 5.0f));

		// 3色でループさせる
		int colorIndex = colorGroup % 3;
		if (colorIndex < 0) {
			colorIndex += 3;
		}

		// 計算された色を割り当て
		float r = colors[colorIndex][0];
		float g = colors[colorIndex][1];
		float b = colors[colorIndex][2];

		// i番目専用のモデルに色をセットし、描画する！
		modelTails_[i]->SetNeonColor(tailIntensity_, r, g, b);
		modelTails_[i]->Draw(transformTails_[i], viewProjection, dummyTexture_);
	}


	if (modelRing_) {
		modelRing_->SetNeonColor(neonIntensity_, neonColor_[0], neonColor_[1], neonColor_[2]);
		modelRing_->Draw(worldTransform_, viewProjection, dummyTexture_);
	}


}

void BaseEnemy::DrawImGui() {
#ifdef USE_IMGUI
	// 座標の操作
	ImGui::DragFloat3("Position", &worldTransform_.translation_.x, 0.1f);

	ImGui::Separator();

	// 色と輝度のスライダー
	ImGui::ColorEdit3("Body Color", bodyColor_);
	ImGui::ColorEdit3("Neon Color", neonColor_);
	ImGui::SliderFloat("Neon Intensity", &neonIntensity_, 0.0f, 20.0f);

	ImGui::Separator();
	ImGui::Text("Tail UV Scroll Settings");
	ImGui::ColorEdit3("Tail Color 1", tailColor1_);
	ImGui::ColorEdit3("Tail Color 2", tailColor2_);
	ImGui::ColorEdit3("Tail Color 3", tailColor3_);
	ImGui::SliderFloat("Tail Intensity", &tailIntensity_, 0.0f, 20.0f);
	

#endif
}

// 状態を切り替える関数
void BaseEnemy::ChangeState(BaseEnemyState* newState) {
	// いあの状態を消して、新しい状態を入れる
	if (state_) {
		delete state_;
	}

	state_ = newState;
	state_->SetEnemy(this);
}

// 移動関数
void BaseEnemy::Move(const Vector3& velocity) {
	worldTransform_.translation_.x += velocity.x;
	worldTransform_.translation_.y += velocity.y;
	worldTransform_.translation_.z += velocity.z;
}

// 座標のゲッター
Vector3 BaseEnemy::GetTranslation() const {
	return worldTransform_.translation_;
}

// 時限発動イベントのクリア
void BaseEnemy::ClearTimedCalls() {
	for (TimedCall* timedCall : timedCalls_) {
		delete timedCall;
	}
	timedCalls_.clear();
}

Vector3 BaseEnemy::GetWorldPosition() {
	// ワールド座標を入れる変数
	Vector3 worldPos;
	// ワールド行列の平行移動成分を取得
	worldPos.x = worldTransform_.matWorld_.m[3][0]; 
	worldPos.y = worldTransform_.matWorld_.m[3][1]; 
	worldPos.z = worldTransform_.matWorld_.m[3][2]; 

	return worldPos;
}