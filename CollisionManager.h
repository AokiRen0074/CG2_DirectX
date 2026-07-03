#pragma once
#include <list>
#include "Collider.h"

class CollisionManager {
public:

	// コライダーリストをクリアする関数
	void ClearColliders();

	// コライダーをリストに登録する関数
	void AddCollider(Collider* collider);

	// 全ての当たり判定をチェックする関数
	void CheckAllCollisions();

private:

	// コライダー二つの衝突判定と応答
	void CheckCollisionPair(Collider* colliderA, Collider* colliderB);

	// コライダーリスト
	std::list<Collider*> colliders_;
};