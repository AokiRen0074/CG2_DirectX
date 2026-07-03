#include "CollisionManager.h"
#include <cmath>

void CollisionManager::ClearColliders() {
	// リストを空にする
	colliders_.clear();
}

void CollisionManager::AddCollider(Collider* collider) {
	// リストに追加する
	colliders_.push_back(collider);
}

void CollisionManager::CheckAllCollisions() {
	// リスト内のペアを総当たり
	std::list<Collider*>::iterator itrA = colliders_.begin();
	for (; itrA != colliders_.end(); ++itrA) {
		Collider* colliderA = *itrA;

		std::list<Collider*>::iterator itrB = itrA;
		itrB++;

		for (; itrB != colliders_.end(); ++itrB) {
			Collider* colliderB = *itrB;

			// ペアの当たり判定
			CheckCollisionPair(colliderA, colliderB);
		}
	}
}

void CollisionManager::CheckCollisionPair(Collider* colliderA, Collider* colliderB) {
	// 衝突フィルタリング
	if ((colliderA->GetCollisionAttribute() & colliderB->GetCollisionMask()) == 0 ||
		(colliderB->GetCollisionAttribute() & colliderA->GetCollisionMask()) == 0) {
		return;
	}

	// 座標の取得と距離の計算
	Vector3 posA = colliderA->GetWorldPosition();
	Vector3 posB = colliderB->GetWorldPosition();

	float dx = posB.x - posA.x;
	float dy = posB.y - posA.y;
	float dz = posB.z - posA.z;
	float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

	// 当たり判定
	if (dist <= colliderA->GetRadius() + colliderB->GetRadius()) {
		colliderA->OnCollision();
		colliderB->OnCollision();
	}
}