#pragma once

#include "Object3d.h"
#include "Vector3.h"
#include "ViewProjection.h"
#include "WorldTransform.h"
#include "TextureManager.h"

// 親クラス
#include "BaseCharacter.h"

class PlayerBullet : public BaseCharacter {

public:

	/*----------------------
	弾
	---------------------*/

	// 更新処理
	void Initialize(Object3d* model, const Vector3& position);

	// 更新処理
	void Update();

private:




};