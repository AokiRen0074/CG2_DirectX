#pragma once
#include "Vector3.h"
#include <cstdint>
#include <cmath>

class Collider {
private:
	
	// 衝突判定
	float radius_ = 1.0f;

	// 衝突属性
	uint32_t collisionAttribute_= 0xffffffff;

	// 衝突マスク
	uint32_t collisionMask_ = 0xffffffff;

public:

	//　半径を取得
	float GetRadius() const { return radius_; }

	// 半径を設定
	void SetRadius(float radius) { radius_ = radius; }

	// 衝突属性のゲッター・セッター
	uint32_t GetCollisionAttribute() const { return collisionAttribute_; }
	void SetCollisionAttribute(uint32_t attribute) { collisionAttribute_ = attribute; }

	// 衝突マスク（相手）のゲッター・セッター
	uint32_t GetCollisionMask() const { return collisionMask_; }
	void SetCollisionMask(uint32_t mask) { collisionMask_ = mask; }

	// 衝突時に呼ばれる関数
	virtual void OnCollision() {}

	// ワールド座標を取得
	virtual Vector3 GetWorldPosition() = 0;

	// 対象の座標との距離を計算する
	virtual float GetDistanceTo(const Vector3& targetPos) {
		Vector3 pos = GetWorldPosition();
		float dx = targetPos.x - pos.x;
		float dy = targetPos.y - pos.y;
		float dz = targetPos.z - pos.z;
		return std::sqrt(dx * dx + dy * dy + dz * dz);
	}

};