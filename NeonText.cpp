#include "NeonText.h"
#include <cctype>
// ==========================================
// デストラクタ（メモリ解放）
// ==========================================
NeonText::~NeonText() {
	Clear();
}

// ==========================================
// 初期化
// ==========================================
void NeonText::Initialize(DirectXCommon* dxCommon) {
	dxCommon_ = dxCommon;
}

// ==========================================
// 文字の生成
// ==========================================
void NeonText::Print(const std::string& text, float startX, float startY, float scale) {
	// 新しい文字を作る前に、古い文字があれば消す
	Clear();

	float currentX = startX;
	float letterSpacing = 2.0f * scale;

	for (char c : text) {
		if (c == ' ') {
			currentX += letterSpacing;
			continue;
		}

		CreateLetter(c, currentX, startY, scale);
		currentX += letterSpacing;
	}
}

// ==========================================
// 質感の一括設定
// ==========================================
void NeonText::SetMaterial(float radius, float softness, float intensity, float r, float g, float b, float lengthOffset) {
	radius_ = radius;
	softness_ = softness;
	intensity_ = intensity;
	color_[0] = r;
	color_[1] = g;
	color_[2] = b;
	lengthOffset_ = lengthOffset;
}

// ==========================================
// 更新処理
// ==========================================
void NeonText::Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix) {
	// 1フレームごとに時間を進める (60FPS想定)
	time_ += 1.0f / 60.0f;

	float currentIntensity = intensity_;

	// サイン波が特定の波（0.7以上）に来た時だけ、ランダムでノイズを走らせる
	if (sinf(time_ * 12.0f) > 0.7f) {
		// 0.2 ～ 1.0 の間で激しく明るさがブレる
		float noise = (rand() % 100) / 100.0f;
		currentIntensity *= (0.2f + noise * 0.8f);
	}
	// さらに稀に、一瞬だけ完全に消える（バグったような表現）
	if (rand() % 1000 < 8) {
		currentIntensity = 0.0f;
	}

	for (NeonSign* sign : neonSigns_) {
		// 💥 計算した currentIntensity を各パーツに送る！
		sign->SetMaterial(radius_, softness_, currentIntensity, color_[0], color_[1], color_[2], lengthOffset_);

		sign->UpdateCamera(viewMatrix, projectionMatrix);
	}
}

// ==========================================
// 描画処理
// ==========================================
void NeonText::Draw() {
	for (NeonSign* sign : neonSigns_) {
		sign->Draw();
	}
}

// ==========================================
// メモリのクリーンアップ
// ==========================================
void NeonText::Clear() {
	for (NeonSign* sign : neonSigns_) {
		delete sign; // newした分を忘れずにdelete！
	}
	neonSigns_.clear();
}

// ==========================================
// 1文字ごとの組み立て工場（GameSceneから丸ごと移植！）
// ==========================================
void NeonText::CreateLetter(char c, float baseX, float baseY, float scale) {

	auto addBar = [&](float ox, float oy, float len, float rot) {
		NeonSign* bar = new NeonSign();
		bar->Initialize(dxCommon_);

		float canvasLength = len + 0.5f;
		bar->SetTransform({ baseX + (ox * scale), baseY + (oy * scale), 0.0f }, { 1.0f * scale, canvasLength * scale, 1.0f }, rot);
		bar->SetTubeLength(len * scale);

		neonSigns_.push_back(bar);
		};

	auto vl = [&]() { addBar(-0.75f, 0.0f, 2.0f, 0.0f); };
	auto vr = [&]() { addBar(0.75f, 0.0f, 2.0f, 0.0f); };
	auto vm = [&]() { addBar(0.0f, 0.0f, 2.0f, 0.0f); };
	auto ht = [&]() { addBar(0.0f, 1.0f, 1.5f, 1.57f); };
	auto hm = [&]() { addBar(0.0f, 0.0f, 1.5f, 1.57f); };
	auto hb = [&]() { addBar(0.0f, -1.0f, 1.5f, 1.57f); };
	auto vtl = [&]() { addBar(-0.75f, 0.5f, 1.0f, 0.0f); };
	auto vbl = [&]() { addBar(-0.75f, -0.5f, 1.0f, 0.0f); };
	auto vtr = [&]() { addBar(0.75f, 0.5f, 1.0f, 0.0f); };
	auto vbr = [&]() { addBar(0.75f, -0.5f, 1.0f, 0.0f); };

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


	case '0':
		vl(); vr(); ht(); hb();
		addBar(0.0f, 0.0f, 2.5f, 0.64f); 
		break;
	case '1':
		vr();
		break;
	case '2':
		ht(); vtr(); hm(); vbl(); hb();
		break;
	case '3':
		ht(); vtr(); hm(); vbr(); hb();
		break;
	case '4':
		vtl(); hm(); vr();
		break;
	case '5':
		ht(); vtl(); hm(); vbr(); hb();
		break;
	case '6':
		ht(); vtl(); hm(); vbl(); vbr(); hb();
		break;
	case '7':
		ht(); vtl(); vtr(); vbr(); // 7の形
		break;
	case '8':
		ht(); hm(); hb(); vl(); vr();
		break;
	case '9':
		ht(); hm(); hb(); vtl(); vtr(); vbr();
		break;

	}

}
