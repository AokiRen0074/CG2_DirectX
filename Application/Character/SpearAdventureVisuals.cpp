#include "SpearPrototype.h"

void SpearPrototype::InitializeAdventureModels() {
    const auto initialize = [](auto &models, const char *name) {
        for (auto &model : models) {
            model = std::make_unique<Object3d>();
            model->Initialize("Resources/Spear", name);
        }
    };

    platformModels_.resize(adventure_.stage.platforms.size());
    enemyModels_.resize(adventure_.enemies.size());
    initialize(platformModels_, "floor.obj");
    initialize(sceneryModels_, "bossCrystal.obj");
    for (auto &model : sceneryModels_) {
        model->SetModel("Resources/AL4/Runtime", "background_crystal.obj");
    }
    initialize(enemyModels_, "bossCrystal.obj");
    initialize(bossModels_, "target.obj");
    bossModels_[0]->SetModel("Resources/AL4/Boss", "boss_core.obj");
    bossModels_[1]->SetModel("Resources/AL4/Boss", "boss_emitter.obj");
    for (size_t i = 2; i < 6; ++i) {
        bossModels_[i]->SetModel("Resources/AL4/Boss", "boss_armor.obj");
    }

    bossModels_[6]->SetModel("Resources/Spear", "floor.obj");
    initialize(hazardModels_, "shaft.obj");
    hazardModels_[0]->SetModel("Resources/Spear", "target.obj");
    initialize(minionModels_, "bossCrystal.obj");
    initialize(particleModels_, "body.obj");
    for (size_t i = 0; i < particleModels_.size(); ++i) {
        if (i % 3 == 0) {
            particleModels_[i]->SetModel("Resources/Spear", "laserCore.obj");
        } else if (i % 3 == 1) {
            particleModels_[i]->SetModel("Resources/Spear", "target.obj");
        }
    }

    initialize(impactModels_, "laserCore.obj");
    initialize(dashGaugeModels_, "body.obj");
    dashGaugeModels_[0]->SetModel("Resources/Spear", "backdrop.obj");
    trailCoreModel_ = std::make_unique<Object3d>();
    trailCoreModel_->Initialize("Resources/Spear", "laserCore.obj");
    gateModel_ = std::make_unique<Object3d>();
    gateModel_->Initialize("Resources/Spear", "body.obj");
    warningModel_ = std::make_unique<Object3d>();
    warningModel_->Initialize("Resources/Spear", "bossArmor.obj");
    backdropModel_ = std::make_unique<Object3d>();
    backdropModel_->Initialize("Resources/Spear", "backdrop.obj");
    laserCoreModel_ = std::make_unique<Object3d>();
    laserCoreModel_->Initialize("Resources/Spear", "laserCore.obj");
}

void SpearPrototype::UpdateAdventureModels() {
    const auto sync=[](auto &models,size_t size,const char *asset) {
        models.resize(size);
        for(auto &model:models) if(!model) {
            model=std::make_unique<Object3d>();model->Initialize("Resources/Spear",asset);
        }
    };
    sync(platformModels_,adventure_.stage.platforms.size(),"floor.obj");
    sync(enemyModels_,adventure_.enemies.size(),"bossCrystal.obj");
    const float gaugeY =
        (std::min)(motion_.y + 1.3f,
                   adventure_.cameraY + (adventure_.cameraDistance - 1.2f) * 0.228875f - 0.35f);
    dashGaugeModels_[0]->SetPosition({motion_.x, gaugeY, -1.0f});
    dashGaugeModels_[0]->SetScale({0.42f, 0.15f, 0.025f});
    dashGaugeModels_[0]->SetDirectionalLight({0, 0, 1}, {1, 1, 1}, 1);
    for (size_t i = 1; i < dashGaugeModels_.size(); ++i) {
        const bool available = int(i) <= motion_.charges;
        const bool restored = adventure_.chargeRefillFlash > 0 && int(i) == motion_.charges;
        const float pulse =
            restored ? 1.0f + float(adventure_.chargeRefillFlash) / 36.0f * 0.5f : 1.0f;
        dashGaugeModels_[i]->SetPosition({motion_.x + (float(i) - 2.0f) * 0.24f, gaugeY, -1.2f});
        dashGaugeModels_[i]->SetScale({0.08f * pulse, 0.08f * pulse, 0.025f});
        dashGaugeModels_[i]->SetRotation({0, 0, 0.785f});
        dashGaugeModels_[i]->SetDirectionalLight({0, 0, 1}, {1, 1, 1},
                                                 available ? (restored ? 2.5f : 1.2f) : 0.12f);
    }

    // 背景はカメラに合わせ、明るい攻撃予告と粒子を読みやすくする。
    backdropModel_->SetPosition({adventure_.cameraX, 1.0f, 12.0f});
    backdropModel_->SetScale({40.0f, 25.0f, 0.5f});
    backdropModel_->SetDirectionalLight({0, 0, 1}, {1, 1, 1}, 1.0f);
    // 遠景・中景・近景を異なる速度で流し、カメラ移動から奥行きを感じられるようにする。
    // 繰り返しは画面の外で行い、移動中に結晶が突然現れないようにする。
    for (size_t i = 0; i < sceneryModels_.size(); ++i) {
        const float layer = float(i / 6);
        const float parallax = 0.18f + layer * 0.16f;
        const float anchor = float(i % 6) * 16.0f + layer * 7.0f;
        float relative = std::fmod(anchor - adventure_.cameraX * parallax + 48, 96.0f);
        if (relative < 0) {
            relative += 96;
        }

        auto &model = sceneryModels_[i];
        const float height = 1.8f + float(i % 4) * 0.9f;
        model->SetPosition(
            {adventure_.cameraX + relative - 48, -1.5f + float(i % 3) * 3.0f, 7.7f - layer * 1.2f});
        model->SetScale({0.7f + float(i % 3) * 0.35f, height, 0.35f});
        model->SetRotation({0, float(i) * 0.45f, (i % 2 == 0 ? 1 : -1) * 0.18f});
        model->SetDirectionalLight({0, 0, 1}, Vector3{0.38f, 0.48f, 0.62f},
                                   (0.8f + layer * 0.25f) * 0.5f);
    }

    laserCoreModel_->SetPosition({adventure_.stage.ArenaX(114), -2.6f, -0.85f});
    laserCoreModel_->SetScale({12, 0.035f, 0.02f});
    laserCoreModel_->SetDirectionalLight({0, 0, 1}, {1, 1, 1}, 1.0f);
    for (size_t i = 0; i < platformModels_.size(); ++i) {
        const auto &floor = adventure_.stage.platforms[i];
        const float width = floor.right - floor.left;
        platformModels_[i]->SetModel("Resources/AL4/Runtime",
                                     width < 10 ? "ground_short.obj" : width > 20 ? "ground_long.obj"
                                                                                 : "ground_normal.obj");
        platformModels_[i]->SetPosition(
            {(floor.left + floor.right) * 0.5f, floor.top - 0.49f, 0.8f});
        platformModels_[i]->SetScale({(floor.right - floor.left) * 0.5f, 0.85f, 2.0f});
        const auto &scale = editorSettings_.scales[1];
        auto &transform = platformModels_[i]->GetTransform();
        transform.scale.x *= scale[0];
        transform.scale.y *= scale[1];
        transform.scale.z *= scale[2];
        const auto &rotation = editorSettings_.rotations[1];
        platformModels_[i]->SetRotation(
            {rotation[0] * 0.0174533f, rotation[1] * 0.0174533f, rotation[2] * 0.0174533f});
    }

    // Both encounters use the same silhouette, size, motion and editor adjustments.
    const auto poseRefillTarget = [this](Object3d &model, float x, float y, size_t index) {
        const float phase = float(adventure_.frame) * 0.055f + float(index);
        model.SetPosition({x, y + std::sin(phase) * 0.15f, 0});
        model.SetScale({0.55f, 0.65f, 0.4f});
        model.SetRotation({0, phase * 0.3f, 0.785f + std::sin(phase) * 0.10f});
        const auto &scale = editorSettings_.scales[4];
        const auto &rotation = editorSettings_.rotations[4];
        auto &transform = model.GetTransform();
        transform.scale.x *= scale[0];
        transform.scale.y *= scale[1];
        transform.scale.z *= scale[2];
        transform.rotate.x += rotation[0] * 0.0174533f;
        transform.rotate.y += rotation[1] * 0.0174533f;
        transform.rotate.z += rotation[2] * 0.0174533f;
    };
    for (size_t i = 0; i < enemyModels_.size(); ++i) {
        const auto &enemy = adventure_.enemies[i];
        poseRefillTarget(*enemyModels_[i], enemy.x, enemy.y, i);
    }

    const float phase = float(adventure_.frame) * 0.045f;
    const float exposed=(std::clamp)((adventure_.armorOpening-1)/.65f,0.0f,1.0f);
    const float bossScale = 1.65f-adventure_.bossHitBlend*.18f+exposed*(.13f+std::sin(phase*2)*.04f);
    bossModels_[0]->SetPosition({adventure_.bossX, adventure_.bossY, 0.6f});
    bossModels_[0]->SetScale({bossScale, bossScale, 0.7f});
    bossModels_[0]->SetRotation({0, phase * 0.25f, std::sin(phase) * 0.08f});
    const auto &coreScale = editorSettings_.scales[4];
    bossModels_[0]->SetScale(
        {bossScale * coreScale[0], bossScale * coreScale[1], 0.7f * coreScale[2]});
    const auto &coreRotation = editorSettings_.rotations[4];
    auto &coreTransform = bossModels_[0]->GetTransform();
    coreTransform.rotate.x += coreRotation[0] * 0.0174533f;
    coreTransform.rotate.y += coreRotation[1] * 0.0174533f;
    coreTransform.rotate.z += coreRotation[2] * 0.0174533f;
    bossModels_[1]->SetPosition({adventure_.bossX, adventure_.bossY, -0.35f});
    bossModels_[1]->SetScale({.9f-exposed*.15f,.8f-exposed*.14f,.4f});
    bossModels_[1]->SetRotation({0, 0, phase});

    // 四枚の槍状の装甲。溜め中に開いて、攻撃の前兆を見せる。
    for (size_t i = 2; i < 6; ++i) {
        const float angle = float(i - 2) * 1.570796f + adventure_.bossVisualAngle;
        const float radius = 2.1f + adventure_.phaseFormBlend * 0.4f +
                             adventure_.bossNovaBlend*.6f +
                             adventure_.armorOpening * 0.45f +
                             std::sin(phase * 4) * 0.08f * adventure_.bossChargeBlend;
        bossModels_[i]->SetPosition({adventure_.bossX + std::cos(angle) * radius,
                                     adventure_.bossY + std::sin(angle) * radius, 0.2f});
        bossModels_[i]->SetScale({0.9f + adventure_.phaseFormBlend * 0.2f, 0.65f, 0.65f});
        bossModels_[i]->SetRotation({0, 0, angle + .35f+exposed*.9f});
    }

    // 3DのHPバーも用意し、開発用UIを閉じてもボスの残りHPが読める。
    // カメラの画角が変わってもHPバーを画面上部へ置き、本体と重ならないようにする。
    const float healthY =
        adventure_.cameraY + (adventure_.cameraDistance - 0.7f) * 0.228875f * 0.84f;
    bossModels_[6]->SetPosition({adventure_.stage.ArenaX(114), healthY, -0.5f});
    bossModels_[6]->SetScale({5, 0.16f, 0.1f});
    const float hpWidth = float(adventure_.bossHp) / float(SpearAdventure::bossMaxHp) * 4.9f;
    bossModels_[7]->SetPosition({adventure_.stage.ArenaX(109.1f) + hpWidth, healthY, -0.7f});
    bossModels_[7]->SetScale({(std::max)(0.01f, hpWidth), 0.10f, 0.1f});

    for (size_t i = 0; i < hazardModels_.size(); ++i) {
        const auto &h = adventure_.hazards[i];
        hazardModels_[i]->SetPosition({h.x, h.warning > 0 ? -2.8f : h.y, -0.2f});
        hazardModels_[i]->SetScale(h.warning > 0 ? Vector3{0.45f, 0.04f, 0.2f}
                                   : h.kind == 0 ? Vector3{0.65f, 0.22f, 0.22f}
                                                 : Vector3{0.14f, 0.6f, 0.15f});
        hazardModels_[i]->SetRotation({0, 0, h.kind == 0 ? phase : 0});
        if (h.kind == 2) {
            hazardModels_[i]->SetScale({12, 0.15f + std::sin(phase * 8) * 0.025f, 0.55f});
            hazardModels_[i]->SetRotation({0, 0, 0});
        }
    }

    for (size_t i = 0; i < particleModels_.size(); ++i) {
        const auto &p = adventure_.particles[i];
        const float size = float(p.life) / 30.0f * 0.16f * p.size;
        // プレイヤーの後ろへ描き、破片が増えても自分を見失わないようにする。
        particleModels_[i]->SetPosition({p.x, p.y, 0.8f});
        particleModels_[i]->SetScale(
            {(std::max)(0.01f, size), (std::max)(0.01f, size * 0.7f), 0.04f});
        particleModels_[i]->SetRotation({0, 0, phase * 2.0f + float(i)});
    }

    for (size_t i = 0; i < adventure_.impacts.size(); ++i) {
        const auto &hit = adventure_.impacts[i];
        const float fade = float(hit.life) / (hit.rank == 0 ? 14.0f : 10.0f);
        for (size_t side = 0; side < 2; ++side) {
            auto &model = impactModels_[i * 2 + side];
            model->SetPosition({hit.x, hit.y, 0.1f});
            model->SetScale({(0.7f + float(hit.rank) * 0.12f) * fade, 0.04f * fade, 0.02f});
            model->SetRotation({0, 0, hit.angle + (side == 0 ? 0.55f : -0.55f)});
            model->SetDirectionalLight({0, 0, 1}, {1, 1, 1}, 1);
        }
    }

    trailCoreModel_->SetPosition(
        {motion_.x - motion_.directionX * 1.3f, motion_.y - motion_.directionY * 1.3f, 0.2f});
    trailCoreModel_->SetScale({1.35f + float(adventure_.pierceCount) * 0.18f, 0.025f, 0.025f});
    trailCoreModel_->SetRotation({0, 0, std::atan2(motion_.directionY, motion_.directionX)});
    trailCoreModel_->SetDirectionalLight({0, 0, 1}, {1, 1, 1}, 1);

    for (size_t i = 0; i < minionModels_.size(); ++i) {
        const auto &minion = adventure_.minions[i];
        poseRefillTarget(*minionModels_[i], minion.x, minion.y, i);
    }

    gateModel_->SetPosition({adventure_.GateX() + 0.45f, 1, 0.2f});
    gateModel_->SetScale({0.06f, 4.5f, 0.06f});
    if (adventure_.bossState == SpearAdventure::BossState::DashWarning) {
        const float dx = adventure_.dashTargetX - adventure_.bossX;
        const float dy = adventure_.dashTargetY - adventure_.bossY;
        warningModel_->SetPosition({(adventure_.dashTargetX + adventure_.bossX) * 0.5f,
                                    (adventure_.dashTargetY + adventure_.bossY) * 0.5f, -0.5f});
        warningModel_->SetScale({std::sqrt(dx * dx + dy * dy) * 0.5f, 0.035f, 0.035f});
        warningModel_->SetRotation({0, 0, std::atan2(dy, dx)});
    } else {
        warningModel_->SetPosition({adventure_.stage.ArenaX(114), -2.85f, -0.5f});
        warningModel_->SetScale({11.0f, 0.035f, 0.1f});
        warningModel_->SetRotation({0, 0, 0});
    }

    const auto applyLight = [this](auto &model) {
        const auto &direction = editorSettings_.lightDirection;
        const auto &color = editorSettings_.lightColor;
        model->SetDirectionalLight({direction[0], direction[1], direction[2]},
                                   {color[0], color[1], color[2]}, editorSettings_.lightIntensity);
    };
    const auto applyAll = [&applyLight](auto &models) {
        for (auto &model : models) {
            applyLight(model);
        }
    };
    applyAll(platformModels_);
    applyAll(enemyModels_);
    const auto applyEnemyLight = [this](auto &model) {
        const auto &direction = editorSettings_.lightDirection;
        const auto &color = editorSettings_.lightColor;
        model->SetDirectionalLight({direction[0], direction[1], direction[2]},
                                   {color[0], color[1] * 0.5f, color[2] * 0.2f},
                                   editorSettings_.lightIntensity * 2.0f);
    };

    // 敵は暗い結晶の外殻と橙の核。上昇練習の敵も背景に埋もれないようにする。
    for (auto &model : enemyModels_) {
        applyEnemyLight(model);
    }

    applyAll(bossModels_);
    // 装甲の二色は、エディターのライト色・方向・強度も反映する。
    for (size_t i = 2; i < 6; ++i) {
        const Vector3 tint = i % 2 == 0 ? Vector3{1.0f, 0.5f, 0.2f} : Vector3{0.7f, 0.2f, 0.07f};
        const auto &direction = editorSettings_.lightDirection;
        const auto &color = editorSettings_.lightColor;
        bossModels_[i]->SetDirectionalLight(
            {direction[0], direction[1], direction[2]},
            {tint.x * color[0], tint.y * color[1], tint.z * color[2]},
            editorSettings_.lightIntensity * 1.4f);
    }

    // 本体・装甲・HPも同じ暖色へ寄せ、プレイヤーの赤青と役割を分ける。
    const auto &enemyDirection = editorSettings_.lightDirection;
    for (size_t i : {size_t(0), size_t(1), size_t(7)}) {
        bossModels_[i]->SetDirectionalLight(
            {enemyDirection[0], enemyDirection[1], enemyDirection[2]}, {1.0f, 0.65f, 0.35f},
            editorSettings_.lightIntensity);
    }

    // Keep the Blender-authored metal facets and emissive inlays from each MTL.

    applyAll(hazardModels_);
    applyAll(particleModels_);
    applyAll(minionModels_);
    for (auto &model : minionModels_) {
        applyEnemyLight(model);
    }

    applyLight(gateModel_);
    applyLight(warningModel_);
    warningModel_->SetDirectionalLight({0, 0, 1}, {1, 0.4f, 0.12f}, 1.8f);
}

void SpearPrototype::UpdateSceneLighting() {
    SceneLighting lighting;
    lighting.cameraEnabled = {adventure_.cameraX, adventure_.cameraY, -adventure_.cameraDistance,
                              1};
    size_t casterCount = 0;
    const auto castShadow = [&](Object3d &model, float strength) {
        if (casterCount >= lighting.casters.size()) {
            return;
        }

        // ワールド行列はDraw時に更新されるため、最新の表示用座標から作る。
        const auto &transform = model.GetTransform();
        lighting.casters[casterCount++] = {
            {transform.translate.x, transform.translate.y, transform.translate.z, strength},
            {transform.scale.x * 1.12f, transform.scale.y, transform.scale.z * 1.25f, 0}};
    };
    castShadow(*objects_[0], 0.65f);
    for (size_t i = 0; i < enemyModels_.size(); ++i) {
        if (adventure_.enemies[i].alive) {
            castShadow(*enemyModels_[i], 0.55f);
        }
    }

    if (adventure_.bossState != SpearAdventure::BossState::Dormant &&
        adventure_.bossState != SpearAdventure::BossState::Defeated) {
        castShadow(*bossModels_[0], 0.55f);
    }

    for (size_t i = 0; i < minionModels_.size(); ++i) {
        if (adventure_.minions[i].life > 0) {
            castShadow(*minionModels_[i], 0.55f);
        }
    }

    const auto segmentLight = [](Vector3 start, Vector3 end, float radius, float intensity,
                                 Vector3 color) {
        return SceneLighting::SegmentLight{{start.x, start.y, start.z, radius},
                                           {end.x, end.y, end.z, intensity},
                                           {color.x, color.y, color.z, 0}};
    };
    // 同じBeamTraceを使い、光の届く場所を実際のレーザーと一致させる。
    if (adventure_.bossState == SpearAdventure::BossState::Beam) {
        const auto line = adventure_.BeamTrace();
        const float fade = (std::min)(1.0f, float(adventure_.bossTimer) / 12);
        lighting.lights[0] = segmentLight({line.x1, line.y1, 0.1f}, {line.x2, line.y2, 0.1f}, 4.5f,
                                          fade * 1.2f, {1, 0.14f, 0.015f});
        lighting.lights[1] =
            segmentLight({line.x1, line.y1 + 0.2f, 0.25f}, {line.x2, line.y2 + 0.2f, 0.25f}, 3.8f,
                         fade * 1.5f, {1, 0.4f, 0.12f});
    } else if (adventure_.bossState == SpearAdventure::BossState::BeamWarning) {
        const float duration = float(adventure_.beamWarningDuration);
        const float charge = (std::clamp)(1 - float(adventure_.bossTimer) / duration, 0.0f, 1.0f);
        const Vector3 source = {adventure_.bossX, adventure_.bossY, -0.8f};
        lighting.lights[0] = segmentLight(source, source, 3.5f, charge * 0.8f, {1, 0.4f, 0.12f});
    }

    // 突進も短い青い照り返しを持ち、同じ世界の発光として扱う。
    for (const auto &trail : effects_.trails) {
        if (trail.fade > 0 && trail.count > 1) {
            const auto &a = trail.points[0];
            const auto &b = trail.points[size_t(trail.count - 1)];
            lighting.lights[1] = segmentLight({a.x, a.y, 0.3f}, {b.x, b.y, 0.3f}, 3.0f,
                                              float(trail.fade) / 12 * 0.7f, {0.08f, 0.4f, 1});
        }
    }

    const auto apply = [&](auto &models) {
        for (auto &model : models) {
            model->SetSceneLighting(lighting);
        }
    };
    apply(platformModels_);
    // 遠景は明暗を残して一段暗くし、前景と発光の主役を邪魔しない。
    auto backgroundLighting = lighting;
    backgroundLighting.ambient = {0.09f, 0.11f, 0.16f, 0};
    for (auto &model : sceneryModels_) {
        model->SetSceneLighting(backgroundLighting);
    }

    apply(enemyModels_);
    apply(minionModels_);
    // 画面上の矢印・残り回数・HPは照明の影響を受けず、常に読みやすくする。
    objects_[0]->SetSceneLighting(lighting);
    for (auto &armor : playerArmor_) {
        armor->SetSceneLighting(lighting);
    }
    objects_[2]->SetSceneLighting(lighting);
    objects_[3]->SetSceneLighting(lighting);
    for (size_t i = 0; i < 6; ++i) {
        bossModels_[i]->SetSceneLighting(lighting);
    }
}
