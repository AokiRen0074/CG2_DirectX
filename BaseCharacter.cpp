#include "BaseCharacter.h"

void BaseCharacter::Draw(const ViewProjection& viewProjection) {
    if (model_) {
        model_->Draw(worldTransform_, viewProjection, textureHandle_);
    }
}