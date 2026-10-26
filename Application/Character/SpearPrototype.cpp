#include "SpearPrototype.h"
#include "Input/Input.h"
#include <Windows.h>
#include <Xinput.h>
#include <filesystem>
#include <fstream>
#include <format>
#include "Base/WindowApp.h"
#pragma comment(lib, "Xinput.lib")

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void SpearPrototype::Initialize(DirectXCommon *graphics) {
    screen_.Initialize(graphics);
    LoadUiLayout(true);
    const char *models[] = {"body.obj",   "floor.obj", "shaft.obj", "tip.obj",
                            "target.obj", "shaft.obj", "tip.obj",   "body.obj"};

    for (size_t i = 0; i < objects_.size(); ++i) {
        objects_[i] = std::make_unique<Object3d>();
        objects_[i]->Initialize("Resources/Spear", models[i]);
    }

    objects_[0]->SetModel("Resources/AL4/Player", "core.obj");
    objects_[2]->SetModel("Resources/AL4/Player", "spear_shaft.obj");
    objects_[3]->SetModel("Resources/AL4/Player", "spear_blade.obj");
    for (size_t i = 0; i < playerArmor_.size(); ++i) {
        playerArmor_[i] = std::make_unique<Object3d>();
        playerArmor_[i]->Initialize("Resources/AL4/Player", "armor_" + std::to_string(i) + ".obj");
    }

    LoadSettings();
    LoadStage();
    InitializeAdventureModels();
    const char *sounds[] = {"Sounds/Shot.wav",         "Sounds/hit.wav",
                            "Sounds/BossCharge.wav",   "Sounds/WasAttacked.wav",
                            "Sounds/BigExplosion.wav", "Sounds/warp.wav"};

    for (size_t i = 0; i < effectSounds_.size(); ++i) {
        effectSounds_[i] = Audio::GetInstance()->SoundLoad(sounds[i]);
    }

    motion_.settings = editorSettings_.movement;
    lastUpdate_ = std::chrono::steady_clock::now();
    UpdateObjects();
}

void SpearPrototype::Update() {
    Input *input = Input::GetInstance();
    float directionX = float(input->PushKey(DIK_D) || input->PushKey(DIK_RIGHT)) -
                       float(input->PushKey(DIK_A) || input->PushKey(DIK_LEFT));
    float directionY = float(input->PushKey(DIK_W) || input->PushKey(DIK_UP)) -
                       float(input->PushKey(DIK_S) || input->PushKey(DIK_DOWN));

    XINPUT_STATE pad{};
    bool padJump = false;

    for (DWORD index = 0; index < XUSER_MAX_COUNT; ++index) {
        if (XInputGetState(index, &pad) == ERROR_SUCCESS) {
            const float stickX = float(pad.Gamepad.sThumbLX) / 32768.0f;
            const float stickY = float(pad.Gamepad.sThumbLY) / 32768.0f;

            // 中立付近の揺れで、意図しない突進を起こさない。
            if (stickX * stickX + stickY * stickY > 0.25f * 0.25f) {
                directionX = stickX;
                directionY = stickY;
            }

            padJump = (pad.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
            break;
        }
    }

    // 自動撮影では実キーを混ぜず、同じ条件で演出を比較できるようにする。
    if (fixedPreviewStep_) {
        directionX = directionY = 0;
        padJump = false;
        pendingJump_ = false;
    }

    pendingJump_ |=
        !fixedPreviewStep_ && (input->TriggerKey(DIK_SPACE) || (padJump && !previousPadJump_));
    previousPadJump_ = padJump;
    jumpHeld_ = !fixedPreviewStep_ && (input->PushKey(DIK_SPACE) || padJump);
    const auto now = std::chrono::steady_clock::now();
    accumulator_ +=
        fixedPreviewStep_
            ? 1.0f / 60.0f
            : (std::min)(std::chrono::duration<float>(now - lastUpdate_).count(), 0.10f);
    lastUpdate_ = now;

    if (restartTransition_.Active()) {
        pendingJump_ = false;
        jumpHeld_ = false;
        while (accumulator_ >= 1.0f / 60.0f) {
            accumulator_ -= 1.0f / 60.0f;
            if (restartTransition_.Tick()) ApplyRestart(restartTransition_.fullRun);
        }
        UpdateObjects();
        return;
    }

#ifdef USE_IMGUI
    if (!fixedPreviewStep_ && input->TriggerKey(DIK_F4) && !ImGui::GetIO().WantTextInput) {
        if (uiEditorOpen_) uiEditorOpen_ = false;
        else OpenUiEditor();
        pendingJump_ = jumpHeld_ = false;
        accumulator_ = 0;
    }
    if (uiEditorOpen_) {
        DrawUiEditor();
        pendingJump_ = jumpHeld_ = false;
        accumulator_ = 0;
        UpdateObjects();
        return;
    }
    if (!fixedPreviewStep_ && input->TriggerKey(DIK_F1) && !ImGui::GetIO().WantTextInput) {
        debugHudOpen_ = !debugHudOpen_;
    }
    if (!fixedPreviewStep_ && !stageEditorOpen_ && !ImGui::GetIO().WantTextInput && input->TriggerKey(DIK_F2)) {
        editorOpen_ = !editorOpen_;
        pendingJump_ = false;
        accumulator_ = 0.0f;
    }

    if (!fixedPreviewStep_ && !ImGui::GetIO().WantTextInput && input->TriggerKey(DIK_F3)) {
        stageEditorOpen_ = !stageEditorOpen_;
        editorOpen_ = false;
    }
    if (stageEditorOpen_) {
        DrawStageEditor();
        accumulator_ = 0;
        pendingJump_ = false;
        jumpHeld_ = false;
        UpdateObjects();
        return;
    }

    if (editorOpen_) {
        DrawEditor();
        motion_.settings = editorSettings_.movement;
        // 調整中はゲームを止め、終了時に溜まった時間や入力を消す。
        accumulator_ = 0.0f;
        pendingJump_ = false;
        lastUpdate_ = std::chrono::steady_clock::now();
        UpdateObjects();
        return;
    }

#endif

    if (!fixedPreviewStep_ && input->TriggerKey(DIK_F6)) {
        adventure_.StartBossPractice();
        flow_.Reset();
        effects_ = {};
        pendingJump_ = false;
    }

    if (!fixedPreviewStep_ && input->TriggerKey(DIK_F7)) {
        adventure_.StartBossPractice();
        flow_.Reset();
        adventure_.bossHp = SpearAdventure::phaseThreshold;
        adventure_.BeginSecondPhase();
        effects_ = {};
        pendingJump_ = false;
    }

    const bool mouseLeft = input->PushMouseLeft();
    const bool mousePressed = mouseLeft && !previousMouseLeft_;
    previousMouseLeft_ = mouseLeft;
    POINT cursor{};
    GetCursorPos(&cursor);
    ScreenToClient(WindowApp::GetInstance()->GetHwnd(), &cursor);
    replayHovered_ = UiReplayHovered(float(cursor.x), float(cursor.y));
    if (!fixedPreviewStep_ && flow_.CanReplay() &&
        (input->TriggerKey(DIK_RETURN) || input->TriggerKey(DIK_R) || pendingJump_ ||
         (mousePressed && replayHovered_))) {
        RestartRun();
        return;
    }
    if (!fixedPreviewStep_ && input->TriggerKey(DIK_R) && !adventure_.won) {
        restartTransition_.Begin(false);
        pendingJump_ = false;
        jumpHeld_ = false;
        accumulator_ = 0;
        return;
    }

    while (accumulator_ >= 1.0f / 60.0f) {
        // 命中時だけ短く止める。停止中に押した入力は終了後に受け取る。
        if (flow_.Finished()) {
            effects_.Tick();
            adventure_.Tick(0, 0, false, false);
            pendingJump_ = false;
        } else if (adventure_.hitStop > 0) {
            --adventure_.hitStop;
        } else {
            FixedUpdate(directionX, directionY, pendingJump_);
            pendingJump_ = false;
        }

        flow_.Tick(adventure_.won);
        if (flow_.clearFrames == 180) PlayEffect(1);
        accumulator_ -= 1.0f / 60.0f;
    }

    UpdateObjects();

#ifdef USE_IMGUI
    if (!debugHudOpen_) return;
    ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Always);
    ImGui::Begin("Spear prototype", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("A/D: Move    Space: Jump / Air dash    R: Reset");
    ImGui::TextUnformatted("F2: Lighting / model / movement editor    F3: Stage editor");
    ImGui::TextUnformatted("F6: boss practice / F7: phase II    R: retry from checkpoint");
    ImGui::Text("Air dash: WASD + Space (no direction = no action)");
    ImGui::Text("Controller: Left stick + A");
    ImGui::Text("Air dashes: %d / 3    HP: %d / 6    Score: %d", motion_.charges, adventure_.hp,
                adventure_.score);
    ImGui::Text("Chain: %d    Best: %d    Checkpoint: %d", adventure_.chain,
                adventure_.maximumChain, adventure_.checkpoint);
    ImGui::TextWrapped("%s", adventure_.Lesson());
    ImGui::Text("Pierce: %d   Best pierce: %d   Just dodges: %d", adventure_.pierceCount,
                adventure_.bestPierce, adventure_.dodgeCount);
    if (adventure_.dodgeFlash > 0) {
        ImGui::TextColored(ImVec4(0.4f, 1, 1, 1), "JUST DODGE!");
    }

    if (adventure_.counterFrames > 0) {
        ImGui::TextColored(ImVec4(1, 0.85f, 0.3f, 1),
                           "COUNTER READY: next boss hit deals double damage");
    }

    if (adventure_.bossState != SpearAdventure::BossState::Dormant) {
        ImGui::TextUnformatted("PRISM LANCER");
        ImGui::ProgressBar(float(adventure_.bossHp) / float(SpearAdventure::bossMaxHp),
                           ImVec2(350, 18));
        if (!adventure_.won) {
            ImGui::TextUnformatted(
                adventure_.secondPhase ? "PHASE II: rain / double dash / crystal fan" : "PHASE 1");
        }
    }

    ImGui::TextUnformatted(adventure_.won          ? "STAGE CLEAR"
                           : motion_.grounded      ? "LANDED: charges restored"
                           : motion_.IsAttacking() ? "SPEAR ATTACK"
                                                   : "AIRBORNE");
    ImGui::End();
    std::string feedback;
    ImU32 feedbackColor = IM_COL32(255, 230, 135, 255);
    if (adventure_.dodgeFlash > 0) {
        feedback = "JUST DODGE!";
        feedbackColor = IM_COL32(125, 255, 255, 255);
    } else if (adventure_.pierceFlash > 0 && adventure_.pierceCount >= 2) {
        feedback = "PIERCE x" + std::to_string(adventure_.pierceCount);
    }

    if (!feedback.empty()) {
        ImGui::GetForegroundDrawList()->AddText(
            ImGui::GetFont(), 26, ImVec2(feedbackScreenPoint_[0] - 60, feedbackScreenPoint_[1]),
            feedbackColor, feedback.c_str());
    }

    if (adventure_.chargeRefillFlash > 0) {
        ImGui::GetForegroundDrawList()->AddText(
            ImGui::GetFont(), 18,
            ImVec2(dashGaugeScreenPoint_[0] + 24, dashGaugeScreenPoint_[1] + 12),
            IM_COL32(160, 255, 155, 255), "+1");
    }
#endif
}

void SpearPrototype::FixedUpdate(float directionX, float directionY, bool pressed) {
    effects_.Tick();
    const SpearEffects::Point origin = {motion_.x, motion_.y};
    const bool wasAttacking = motion_.IsAttacking();
    const bool wasGrounded = motion_.grounded;
    const size_t previousImpacts = adventure_.impactCursor;
    const int oldAttack = motion_.attackId;
    const int oldScore = adventure_.score;
    const int oldHp = adventure_.hp;
    const int oldBossHp = adventure_.bossHp;
    const int oldDeaths = adventure_.deaths;
    const auto oldBossState = adventure_.bossState;
    const bool wasWon = adventure_.won;
    const int oldDodges = adventure_.dodgeCount;
    const int oldVolley=adventure_.bossDashSequence;
    const int oldGroundImpact=adventure_.groundImpactSerial;
    bossImpactSoundCooldown_=(std::max)(0,bossImpactSoundCooldown_-1);
    adventure_.Tick(directionX, directionY, pressed, jumpHeld_);
    if (adventure_.deaths > oldDeaths) {
        effects_ = {};
    } else {
        effects_.Movement(origin, wasGrounded, motion_, adventure_.landingEffect);
        if (motion_.attackId != oldAttack) {
            effects_.BeginDash(motion_, origin);
        }

        if (motion_.IsAttacking() || wasAttacking || motion_.attackId != oldAttack) {
            effects_.Record(motion_);
        }

        for (size_t i = previousImpacts; i < adventure_.impactCursor; ++i) {
            const auto &hit = adventure_.impacts[i % adventure_.impacts.size()];
            effects_.Hit(hit.x, hit.y, std::atan2(motion_.directionY, motion_.directionX),
                         hit.rank);
        }

        if (adventure_.bossHp < oldBossHp) {
            effects_.Hit(adventure_.bossX, adventure_.bossY,
                         std::atan2(motion_.directionY, motion_.directionX));
        }
    }

    if (adventure_.hazards[0].life > 0 && adventure_.hazards[0].kind == 2 &&
        adventure_.scorchedFloorEnabled) {
        effects_.groundScorch = {adventure_.floorLaserDuration - adventure_.hazards[0].life,
                                 adventure_.floorLaserDuration, adventure_.hazards[0].x};
    } else if (adventure_.won || !adventure_.scorchedFloorEnabled ||
               adventure_.bossState == SpearAdventure::BossState::PhaseChange) {
        effects_.groundScorch.age = -1;
    }

    int effect = motion_.attackId != oldAttack ? 0 : -1;

    if (adventure_.bossState != oldBossState &&
        (adventure_.bossState == SpearAdventure::BossState::DashWarning ||
         adventure_.bossState == SpearAdventure::BossState::LaserWarning ||
         adventure_.bossState == SpearAdventure::BossState::SummonWarning ||
         adventure_.bossState == SpearAdventure::BossState::BeamWarning ||
         adventure_.bossState == SpearAdventure::BossState::FanWarning ||
         adventure_.bossState == SpearAdventure::BossState::RainWarning ||
         adventure_.bossState == SpearAdventure::BossState::NovaWarning ||
         adventure_.bossState == SpearAdventure::BossState::PhaseChange)) {
        effect = 2;
    }

    if (adventure_.bossState != oldBossState &&
        (adventure_.bossState == SpearAdventure::BossState::Laser ||
         adventure_.bossState == SpearAdventure::BossState::Beam)) {
        effect = 0;
    }

    if(adventure_.bossDashSequence>oldVolley &&
        (adventure_.bossState==SpearAdventure::BossState::Fan || adventure_.bossState==SpearAdventure::BossState::Nova ||
         adventure_.bossState==SpearAdventure::BossState::Rain)) effect=0;
    if(adventure_.groundImpactSerial>oldGroundImpact && bossImpactSoundCooldown_==0) {
        effect=1;bossImpactSoundCooldown_=12;
    }
    if (adventure_.score > oldScore) {
        effect = 1;
    }

    if (adventure_.dodgeCount > oldDodges) {
        effect = 5;
    }

    if (adventure_.hp < oldHp || adventure_.deaths > oldDeaths) {
        effect = 3;
    }

    if (adventure_.bossState == SpearAdventure::BossState::PhaseChange &&
        oldBossState != SpearAdventure::BossState::PhaseChange) {
        effect = 2;
    }

    if (!wasWon && adventure_.won) {
        motion_.dashFrames = 0;
        motion_.velocityX = motion_.velocityY = 0;
        if (adventure_.bossState != SpearAdventure::BossState::Defeated) {
            adventure_.defeatX = motion_.x;
            adventure_.defeatY = motion_.y;
        }
        effect = 4;
    }

    if (effect >= 0) {
        PlayEffect(size_t(effect));
    }

    landingFlash_ = (std::max)(0, landingFlash_ - 1);

    if (adventure_.landingEffect) {
        landingFlash_ = 12;
    }
}

SpearPrototype::~SpearPrototype() {
    Audio::GetInstance()->SoundStopWave(effectVoice_);

    for (auto &sound : effectSounds_) {
        Audio::GetInstance()->SoundUnload(&sound);
    }
}

void SpearPrototype::PlayEffect(size_t index) {
    // 前の音を止めてから再生するので、波形の寿命とボイスの管理が一致する。
    Audio::GetInstance()->SoundStopWave(effectVoice_);
    effectVoice_ = Audio::GetInstance()->SoundPlay(effectSounds_[index]);
    effectVoice_->SetVolume(0.35f);
    if (index == 1) {
        effectVoice_->SetVolume(0.35f + float((std::min)(4, adventure_.pierceCount)) * 0.035f);
        effectVoice_->SetFrequencyRatio(1.0f +
                                        float((std::min)(4, adventure_.pierceCount)) * 0.10f);
    }
}

void SpearPrototype::UpdateObjects() {
    const float time = float(adventure_.frame) / 60.0f;
    const float bob = motion_.grounded ? std::sin(time * 2.8f) * 0.045f : 0;
    const float bank = motion_.IsAttacking() ? -motion_.directionX * 0.22f
                                            : -motion_.velocityX * 0.55f;
    objects_[0]->SetPosition({motion_.x, motion_.y + 0.25f + bob, 0});
    objects_[0]->SetScale({0.62f, 0.62f, 0.62f});
    objects_[0]->SetRotation({0, 0, bank});

    const float weaponX = motion_.IsAttacking() ? motion_.directionX : motion_.aimX;
    const float weaponY = motion_.IsAttacking() ? motion_.directionY : motion_.aimY;
    const float angle = std::atan2(weaponY, weaponX);
    objects_[2]->SetPosition({motion_.x + weaponX * 0.82f,
                              motion_.y + 0.35f + bob + weaponY * 0.82f, -0.32f});
    objects_[2]->SetScale({0.82f, 0.82f, 0.82f});
    objects_[2]->SetRotation({0, 0, angle});
    objects_[3]->SetPosition({motion_.x + weaponX * 1.88f,
                              motion_.y + 0.35f + bob + weaponY * 1.88f, -0.32f});
    objects_[3]->SetScale({0.82f, 0.82f, 0.82f});
    objects_[3]->SetRotation({0, 0, angle});

    objects_[4]->SetPosition({4.0f, 0.0f, 0.0f});
    const float targetScale = 0.85f;
    objects_[4]->SetScale({targetScale, targetScale, targetScale});
    objects_[4]->SetRotation({0.0f, 0.0f, 0.35f});

    // 常時表示する矢印。最後の向きを保持しても、中立入力で突進はしない。
    const float aimAngle = std::atan2(motion_.aimY, motion_.aimX);
    const float indicatorTop =
        adventure_.cameraY + (adventure_.cameraDistance - 0.5f) * 0.228875f - 0.8f;
    const float indicatorY =
        (std::min)(motion_.y + 0.95f, indicatorTop - (std::max)(0.0f, motion_.aimY) * 1.06f);
    objects_[5]->SetPosition(
        {motion_.x + motion_.aimX * 0.72f, indicatorY + motion_.aimY * 0.72f, -0.5f});
    objects_[5]->SetScale({0.20f, 0.04f, 0.04f});
    objects_[5]->SetRotation({0.0f, 0.0f, aimAngle});
    objects_[6]->SetPosition(
        {motion_.x + motion_.aimX * 1.06f, indicatorY + motion_.aimY * 1.06f, -0.5f});
    objects_[6]->SetScale({0.18f, 0.14f, 0.08f});
    objects_[6]->SetRotation({0.0f, 0.0f, aimAngle + 3.14159265f});

    // 薄く長い軌跡で、突進の速度と方向を示す。
    objects_[7]->SetPosition(
        {motion_.x - motion_.directionX * 1.15f, motion_.y - motion_.directionY * 1.15f, 0.4f});
    objects_[7]->SetScale({1.3f + float(adventure_.pierceCount) * 0.18f,
                           0.06f + float(adventure_.pierceCount) * 0.01f, 0.055f});
    objects_[7]->SetRotation({0.0f, 0.0f, angle});

    // ワールド行列はDrawが更新する。モデルごとの調整UIは開かない。
    for (size_t i = 0; i < objects_.size(); ++i) {
        auto &transform = objects_[i]->GetTransform();
        const auto &scale = editorSettings_.scales[i];
        const auto &rotation = editorSettings_.rotations[i];
        transform.scale.x *= scale[0];
        transform.scale.y *= scale[1];
        transform.scale.z *= scale[2];
        constexpr float degreesToRadians = 3.14159265f / 180.0f;
        transform.rotate.x += rotation[0] * degreesToRadians;
        transform.rotate.y += rotation[1] * degreesToRadians;
        transform.rotate.z += rotation[2] * degreesToRadians;
        const auto &light = editorSettings_.lightDirection;
        const auto &color = editorSettings_.lightColor;
        objects_[i]->SetDirectionalLight({light[0], light[1], light[2]},
                                         {color[0], color[1], color[2]},
                                         editorSettings_.lightIntensity);
    }

    // Armor keeps its own facets and gently floats around the core.
    for (size_t i = 0; i < playerArmor_.size(); ++i) {
        const float phase = time * 2.2f + float(i) * 1.7f;
        auto transform = objects_[0]->GetTransform();
        transform.translate.x += std::sin(phase) * 0.025f;
        transform.translate.y += std::cos(phase) * 0.035f;
        transform.rotate.z += std::sin(phase * 0.7f) * 0.025f;
        playerArmor_[i]->GetTransform() = transform;
        const auto &light = editorSettings_.lightDirection;
        const auto &color = editorSettings_.lightColor;
        playerArmor_[i]->SetDirectionalLight({light[0], light[1], light[2]},
                                             {color[0], color[1], color[2]},
                                             editorSettings_.lightIntensity);
    }
    UpdateAdventureModels();
    UpdateSceneLighting();
}

void SpearPrototype::DrawEditor() {
#ifdef USE_IMGUI
    ImGui::SetNextWindowPos(ImVec2(820.0f, 20.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(440.0f, 660.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowCollapsed(false, ImGuiCond_Appearing);
    ImGui::Begin("Spear editor", &editorOpen_);
    ImGui::TextUnformatted("Game paused. F2: close and test.");

    if (ImGui::Button("Save")) {
        SaveSettings();
    }

    ImGui::SameLine();
    if (ImGui::Button("Reload")) {
        LoadSettings();
    }

    ImGui::SameLine();

    if (ImGui::Button("Defaults")) {
        editorSettings_ = SpearEditorSettings{};
        editorStatus_ = "Defaults applied. Save to keep them.";
    }

    ImGui::TextWrapped("%s", editorStatus_.c_str());
    ImGui::Checkbox("Fracture floor attack (trial)", &adventure_.scorchedFloorEnabled);
    ImGui::TextWrapped("Off: compare with the horizontal laser layout. Applies to this session.");

    if (ImGui::CollapsingHeader("Directional light", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::SliderFloat3("Direction", editorSettings_.lightDirection.data(), -1.0f, 1.0f, "%.3f",
                            ImGuiSliderFlags_AlwaysClamp);
        ImGui::ColorEdit3("Light color", editorSettings_.lightColor.data());
        ImGui::SliderFloat("Intensity", &editorSettings_.lightIntensity, 0.0f, 10.0f, "%.3f",
                           ImGuiSliderFlags_AlwaysClamp);
    }

    if (ImGui::CollapsingHeader("Model appearance", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char *names[] = {"Player",
                               "Platforms",
                               "Spear shaft",
                               "Spear tip",
                               "Enemies / boss core",
                               "Direction arrow shaft",
                               "Direction arrow tip",
                               "Dash trail"};
        ImGui::Combo("Object", &selectedObject_, names, IM_ARRAYSIZE(names));
        auto &scale = editorSettings_.scales[size_t(selectedObject_)];
        auto &rotation = editorSettings_.rotations[size_t(selectedObject_)];
        ImGui::SliderFloat3("Scale multiplier", scale.data(), 0.05f, 5.0f, "%.3f",
                            ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat3("Rotation (degrees)", rotation.data(), -180.0f, 180.0f, "%.3f",
                            ImGuiSliderFlags_AlwaysClamp);
        ImGui::TextWrapped("Appearance only. Movement and collision sizes stay the same.");

        if (ImGui::Button("Reset selected model")) {
            scale = {1.0f, 1.0f, 1.0f};
            rotation = {0.0f, 0.0f, 0.0f};
        }
    }

    if (ImGui::CollapsingHeader("Movement feel", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto &m = editorSettings_.movement;
        ImGui::TextUnformatted("Values per 60Hz frame; higher accel/brake = quicker response.");
        ImGui::SliderFloat("Move speed", &m.moveSpeed, 0.05f, 0.30f, "%.3f",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Ground acceleration", &m.groundAcceleration, 0.005f, 0.10f, "%.3f",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Ground braking", &m.groundBraking, 0.005f, 0.20f, "%.3f",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Air acceleration", &m.airAcceleration, 0.005f, 0.10f, "%.3f",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Air braking", &m.airBraking, 0.005f, 0.10f, "%.3f",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Jump speed", &m.jumpSpeed, 0.15f, 0.50f, "%.3f",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Rising gravity", &m.risingGravity, 0.002f, 0.04f, "%.3f",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Apex gravity", &m.apexGravity, 0.002f, 0.02f, "%.3f",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Falling gravity", &m.fallingGravity, 0.005f, 0.06f, "%.3f",
                           ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderFloat("Dash speed", &m.dashSpeed, 0.30f, 1.0f, "%.3f",
                           ImGuiSliderFlags_AlwaysClamp);
    }

    ImGui::End();
#endif
}

void SpearPrototype::LoadSettings() {
    const std::filesystem::path path = "Settings/spear-editor.json";

    try {
        if (!std::filesystem::exists(path)) {
            editorStatus_ = "No saved settings yet. Save creates Settings/spear-editor.json.";
            return;
        }

        std::ifstream file(path);
        nlohmann::json json;
        file >> json;
        // 全項目の検証が済んでから適用し、失敗時は現在の調整を維持する。
        editorSettings_ = SpearEditorSettings::Parse(json);
        editorStatus_ = "Settings loaded.";
    } catch (const std::exception &error) {
        editorStatus_ = std::string("Load failed: ") + error.what();
    }
}

void SpearPrototype::SaveSettings() {
    const std::filesystem::path path = "Settings/spear-editor.json";
    const std::filesystem::path temporary = "Settings/spear-editor.json.tmp";

    try {
        const auto json = editorSettings_.Serialize();
        (void)SpearEditorSettings::Parse(json);
        std::filesystem::create_directories(path.parent_path());
        std::ofstream file(temporary, std::ios::trunc);
        file << json.dump(2) << '\n';
        file.close();

        if (!file) {
            throw std::runtime_error("Could not write settings");
        }

        if (std::filesystem::exists(path)) {
            std::filesystem::copy_file(path, "Settings/spear-editor.json.bak",
                                       std::filesystem::copy_options::overwrite_existing);
        }

        if (!MoveFileExW(temporary.c_str(), path.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            throw std::runtime_error("Could not replace settings file");
        }

        editorStatus_ = "Saved. Previous settings: spear-editor.json.bak";
    } catch (const std::exception &error) {
        editorStatus_ = std::string("Save failed: ") + error.what();
    }
}

void SpearPrototype::Draw(const ViewProjection &camera) {
    ViewProjection stageCamera = camera;
    stageCamera.translation_.x = adventure_.cameraX;
    stageCamera.translation_.y = adventure_.cameraY;
    stageCamera.translation_.z = -adventure_.cameraDistance;
    if (flow_.Finished()) {
        const float focus = SpearGameFlow::Ease(8,100,float(flow_.clearFrames));
        stageCamera.translation_.x += (adventure_.defeatX-adventure_.cameraX)*focus*.45f;
        stageCamera.translation_.y += (adventure_.defeatY-adventure_.cameraY)*focus*.25f;
        stageCamera.translation_.z += focus*4.0f;
    }
    // 撃破の反動は数ピクセルだけ攻撃方向へ動かし、すぐ滑らかに戻す。
    stageCamera.translation_.x += effects_.kickX;
    stageCamera.translation_.y += effects_.kickY;

    // 短い左右・上下の揺れだけを加え、奥行きと追従位置は変えない。
    if (adventure_.shake > 0) {
        const float envelope = float(adventure_.shake) / 18.0f;
        stageCamera.translation_.x +=
            SpearAdventure::ShakeNoise(float(adventure_.frame) * 0.32f, 1) * envelope * 0.16f;
        stageCamera.translation_.y +=
            SpearAdventure::ShakeNoise(float(adventure_.frame) * 0.32f, 7) * envelope * 0.09f;
    }

    stageCamera.UpdateMatrix();
    dashGaugeCamera_ = stageCamera;
    const float gaugeY =
        (std::min)(motion_.y + 1.3f,
                   adventure_.cameraY + (adventure_.cameraDistance - 1.2f) * 0.228875f - 0.35f);
    const Vector3 gaugePoint = Transform({motion_.x, gaugeY, -1.2f},
                                         Multiply(stageCamera.matView, stageCamera.matProjection));
    dashGaugeScreenPoint_ = {(gaugePoint.x * 0.5f + 0.5f) * 1280,
                             (0.5f - gaugePoint.y * 0.5f) * 720};
    const Vector3 playerLabel = Transform({motion_.x, motion_.y + 2.8f, 0},
                                          Multiply(stageCamera.matView, stageCamera.matProjection));
    feedbackScreenPoint_ = {(std::clamp)((playerLabel.x * 0.5f + 0.5f) * 1280, 80.0f, 1180.0f),
                            (std::clamp)((0.5f - playerLabel.y * 0.5f) * 720, 40.0f, 660.0f)};
    const Vector3 focus = Transform({adventure_.defeatX, adventure_.defeatY, 0},
                                    Multiply(stageCamera.matView, stageCamera.matProjection));
    victoryBlurCenter_ = {(std::clamp)(focus.x * 0.5f + 0.5f, 0.0f, 1.0f),
                          (std::clamp)(0.5f - focus.y * 0.5f, 0.0f, 1.0f)};
    backdropModel_->Draw(stageCamera);
    for (auto &model : sceneryModels_) {
        model->Draw(stageCamera);
    }

    for (auto &platform : platformModels_) {
        platform->Draw(stageCamera);
    }

    for (size_t i = 0; i < enemyModels_.size(); ++i) {
        if (adventure_.enemies[i].alive) {
            enemyModels_[i]->Draw(stageCamera);
        }
    }

    if (adventure_.bossState != SpearAdventure::BossState::Dormant && !adventure_.won) {
        for (size_t i = 0; i < 6; ++i) {
            if (adventure_.bossFlash > 0 && adventure_.bossFlash % 4 < 2 && i == 0) {
                continue;
            }

            bossModels_[i]->Draw(stageCamera);
        }
    }

    for (size_t i = 0; i < hazardModels_.size(); ++i) {
        if (adventure_.hazards[i].life > 0 && adventure_.hazards[i].kind < 2) {
            hazardModels_[i]->Draw(stageCamera);
        }
    }

    // 粒子は柔らかい光点としてHDRエフェクト描画へ渡す。

    for (size_t i = 0; i < minionModels_.size(); ++i) {
        if (adventure_.minions[i].life > 0) {
            minionModels_[i]->Draw(stageCamera);
        }
    }

    // レーザーの芯と外光は、放電を含むHDR描画へ移す。

    // 命中の光と弧はHDRエフェクト描画へ移す。

    // 光の軌跡は実際の経路に沿うHDRの帯へ移す。

    if (adventure_.GateX() < 130 && adventure_.bossState == SpearAdventure::BossState::Dormant) {
        gateModel_->Draw(stageCamera);
    }

    if (adventure_.bossState == SpearAdventure::BossState::DashWarning ||
        (adventure_.bossState == SpearAdventure::BossState::LaserWarning &&
         !adventure_.scorchedFloorEnabled)) {
        warningModel_->Draw(stageCamera);
    }

    for (size_t i = 0; i < objects_.size(); ++i) {
        // 元のテスト用の地面・的はコースの足場と敵へ置き換える。
        if (i == 1 || i == 4 || i == 5 || i == 6) {
            continue;
        }

        if (!restartTransition_.Active() && adventure_.invincibility > 0 && adventure_.invincibility % 6 < 2 &&
            (i == 0 || i == 2 || i == 3)) {
            continue;
        }

        if (i == 7) {
            continue;
        }

        objects_[i]->Draw(stageCamera);
    }
    if (restartTransition_.Active() || !(adventure_.invincibility > 0 && adventure_.invincibility % 6 < 2)) {
        for (auto &armor : playerArmor_) {
            armor->Draw(stageCamera);
        }
    }
}

bool SpearPrototype::DrawEffects(EffectRenderer &renderer) {
    renderer.Begin();
    DrawBossOverhaulEffects(renderer);
    if (flow_.Finished() && flow_.clearFrames < 110) {
        const float t = float(flow_.clearFrames);
        const float fade = 1 - SpearGameFlow::Ease(35,110,t);
        const Vector3 center{adventure_.defeatX,adventure_.defeatY,-0.1f};
        renderer.AddArc(center,t*.015f,.6f+t*.085f,6.1f,.07f*fade,{.1f,1.2f,2.3f,fade*.8f});
        renderer.AddArc(center,-t*.02f,.3f+t*.06f,5.7f,.035f*fade,{2.4f,.5f,.08f,fade*.6f});
        for (int i=0;i<24;++i) {
            const float a=float(i)*2.399963f;
            const float distance=(.4f+float(i%5)*.13f)*t*.11f;
            const Vector3 p{center.x+std::cos(a)*distance,
                            center.y+std::sin(a)*distance-t*t*.00075f,0.2f};
            renderer.AddCrystal(p,a+t*.045f,.045f+float(i%3)*.025f,
                                 i%3==0?Vector4{2.1f,.45f,.08f,fade}:Vector4{.2f,1.4f,2.6f,fade});
        }
        if(t<12) {
            const float flash=1-t/12;
            renderer.AddGlow(center,{1,0,0},6.0f,.065f,{2.0f,2.5f,3.0f,flash});
            renderer.AddGlow(center,{0,1,0},3.3f,.045f,{1.6f,2.2f,3.0f,flash});
        }
    }
    const bool playerVisible = restartTransition_.Active() || !(adventure_.invincibility > 0 && adventure_.invincibility % 6 < 2);
    if (playerVisible) {
        const float pulse = 1.8f + std::sin(float(adventure_.frame) * 0.035f) * 0.12f;
        renderer.AddModelEmission(*objects_[0]->GetModel(), objects_[0]->GetWorldMatrix(), pulse);
        renderer.AddModelEmission(*objects_[3]->GetModel(), objects_[3]->GetWorldMatrix(),
                                  motion_.IsAttacking() ? 2.3f : 1.8f);
        const auto center = objects_[0]->GetTransform().translate;
        renderer.AddGlow({center.x, center.y, -0.28f}, {0, 1, 0}, 0.68f, 0.28f,
                         {0.015f, 0.65f, 1.4f, 0.15f});
    }
    // 床の持続レーザーも白い芯・赤い外光・不規則な放電に統一する。
    if (!adventure_.scorchedFloorEnabled && adventure_.hazards[0].life > 0 &&
        adventure_.hazards[0].kind == 2) {
        const float fade = (std::min)(1.0f, float(adventure_.hazards[0].life) / 10);
        renderer.AddElectricBeam({80, -2.6f, 0.1f}, {150, -2.6f, 0.1f}, adventure_.frame,
                                 0.45f + fade * 0.55f);
    }

    const bool fanWarning = adventure_.bossState == SpearAdventure::BossState::FanWarning;
    if (fanWarning) {
        const int count = adventure_.FanCount();
        for (int i = 0; i < count; ++i) {
            const float angle =
                adventure_.fanAimAngle + (float(i) - float(count - 1) * 0.5f) * adventure_.FanSpacing();
            const Vector3 source = {adventure_.bossX, adventure_.bossY, 0.1f};
            renderer.AddBeam(
                source,
                {source.x + std::cos(angle) * 18, source.y + std::sin(angle) * 18, source.z},
                0.014f, {2.4f, 0.5f, 0.12f, 0.2f});
        }

        renderer.AddLaserEmitter({adventure_.bossX, adventure_.bossY, -0.9f},
                                 adventure_.fanAimAngle, adventure_.frame,
                                 1 - float(adventure_.bossTimer) / 75, false);
    }

    if (adventure_.bossState == SpearAdventure::BossState::RainWarning) {
        for (int lane = 0; lane < 8; ++lane) {
            if (adventure_.RainSafe(lane,0)) {
                continue;
            }

            const float x = adventure_.RainLaneX(lane);
            const float pulse = 0.2f + (std::sin(float(adventure_.frame) * 0.18f) + 1) * 0.08f;
            renderer.AddBeam({x, 9.7f, 0.2f}, {x, -3.48f, 0.2f}, 0.012f,
                             {2.4f, 0.5f, 0.12f, pulse});
            renderer.AddGlow({x, -3.35f, -0.5f}, {1, 0, 0}, 0.3f, 0.05f,
                             {2.8f, 0.65f, 0.15f, pulse * 2});
            renderer.AddCrystal({x, 9.7f, 0.1f}, -1.570796f, 0.3f, {2.8f, 0.7f, 0.18f, 0.7f});
        }
    }

    for (const auto &hazard : adventure_.hazards) {
        if (hazard.life <= 0 || hazard.kind < 3) {
            continue;
        }

        if(hazard.warning>0) {
            renderer.AddBeam({hazard.x,9.7f,.1f},{hazard.x,-3.48f,.1f},.017f,{2.6f,.55f,.1f,.3f});
            renderer.AddCrystal({hazard.x,9.7f,.1f},-1.570796f,.4f,{2.7f,.65f,.1f,.5f});
            continue;
        }
        const float angle = std::atan2(hazard.vy, hazard.vx);
        const float length=(std::max)(.001f,std::sqrt(hazard.vx*hazard.vx+hazard.vy*hazard.vy));
        const float tail=hazard.kind==3?2.2f:1.25f;
        const float fade=(std::min)(1.0f,float(hazard.life)/12);
        renderer.AddBeam({hazard.x-hazard.vx/length*tail,hazard.y-hazard.vy/length*tail,.2f},
            {hazard.x,hazard.y,.2f},hazard.kind==3?.055f:.035f,{2.8f,.6f,.12f,fade*.55f});
        const Vector3 position = {hazard.x, hazard.y, 0.1f};
        renderer.AddGlow(position, {hazard.vx, hazard.vy, 0}, hazard.kind == 3 ? 0.65f : 0.42f,
                         0.12f, {2.6f, 0.45f, 0.08f, 0.65f});
        renderer.AddCrystal(position, angle, hazard.kind == 3 ? 0.46f : 0.3f,
                            {2.8f, 0.85f, 0.22f, 0.9f});
        renderer.AddGlow(position, {hazard.vx, hazard.vy, 0}, 0.18f, 0.025f,
                         {3, 1.8f, 0.85f, 0.7f});
    }

    const bool floorWarning = adventure_.bossState == SpearAdventure::BossState::LaserWarning;
    const bool floorActive = adventure_.bossState == SpearAdventure::BossState::Laser;
    const auto &scorch = effects_.groundScorch;
    if (floorWarning || floorActive) {
        float charge =
            floorWarning ? (std::clamp)(1 - float(adventure_.bossTimer) / 75, 0.0f, 1.0f) : 1;
        const bool striking = floorActive && (!adventure_.scorchedFloorEnabled || scorch.age < 14);
        if (floorActive && !striking) {
            charge = 0.25f;
        }

        const float aim = adventure_.scorchedFloorEnabled
                              ? std::atan2(-3.48f - adventure_.bossY, 114 - adventure_.bossX)
                              : -1.570796f;
        renderer.AddLaserEmitter({adventure_.bossX, adventure_.bossY, -0.9f}, aim, adventure_.frame,
                                 charge, striking);
    }

    if (adventure_.scorchedFloorEnabled && scorch.age >= 0) {
        const float fade =
            scorch.age < scorch.duration
                ? 1.0f
                : (std::clamp)(1 - float(scorch.age - scorch.duration) / 24, 0.0f, 1.0f);
        const float radius = (std::min)(12.0f, float(scorch.age + 1) * 1.4f);
        renderer.AddScorchedGround({scorch.centerX, -3.48f, 0.6f}, radius, scorch.age, fade,
                                   adventure_.frame);
        // 撃ち込みは短く。以後は床の亀裂と噴出する光が攻撃の主体になる。
        if (floorActive && scorch.age < 14) {
            const float impact = 1 - float(scorch.age) / 14;
            if (scorch.age < 5) {
                const float flash = 1 - float(scorch.age) / 5;
                renderer.AddGlow({scorch.centerX, -3.3f, 0.2f}, {1, 0, 0}, 3.3f, 0.09f,
                                 {3.5f, 2.2f, 0.7f, flash});
            }

            renderer.AddElectricBeam({adventure_.bossX, adventure_.bossY, 0.1f},
                                     {scorch.centerX, -3.48f, 0.1f}, adventure_.frame,
                                     impact * 1.15f);
            renderer.AddGlow({scorch.centerX, -3.35f, -0.2f}, {1, 0, 0}, 2.4f, 0.55f,
                             {2.8f, 0.6f, 0.025f, impact * 0.8f});
        }
    }

    const bool beamWarning = adventure_.bossState == SpearAdventure::BossState::BeamWarning;
    const bool beamActive = adventure_.bossState == SpearAdventure::BossState::Beam;
    if (beamWarning || beamActive) {
        const Vector3 emitter = {adventure_.bossX, adventure_.bossY, -0.9f};
        const float rotation = float(adventure_.frame) * 0.12f;
        const float charge = beamWarning
                                 ? (std::clamp)(1 - float(adventure_.bossTimer) /
                                                        float(adventure_.beamWarningDuration),
                                                0.0f, 1.0f)
                                 : 1;
        renderer.AddLaserEmitter(emitter, adventure_.beamAimAngle, adventure_.frame, charge,
                                 beamActive);
        if (beamWarning) {
            // 発射方向を追い続けず、予告した範囲を読んで避けられるようにする。
            for (int i = -1; i <= 1; ++i) {
                const auto line = adventure_.BeamTrace(adventure_.beamAimAngle + float(i) * adventure_.BeamSweepHalfAngle());
                renderer.AddBeam({line.x1, line.y1, -0.9f}, {line.x2, line.y2, -0.9f}, 0.013f,
                                 {2.2f, 0.55f, 0.03f, i == 0 ? 0.4f : 0.15f});
            }
        } else {
            const auto line = adventure_.BeamTrace();
            const float fade = (std::min)(1.0f, float(adventure_.bossTimer) / 12);
            renderer.AddElectricBeam({line.x1, line.y1, 0.1f}, {line.x2, line.y2, 0.1f},
                                     adventure_.frame, 0.5f + fade * 0.5f);
            const Vector3 contact = {line.x2, line.y2, 0.05f};
            renderer.AddGlow(contact, {1, 0, 0}, 0.7f, 0.18f, {3, 0.5f, 0.025f, fade * 0.7f});
            renderer.AddArc(contact, rotation, 0.4f, 2.2f, 0.035f, {3, 1.0f, 0.1f, fade * 0.65f});
            // 地面との接点から火花を跳ね上げ、終端を光の溜まりとして見せる。
            if (line.y2 < -3.29f) {
                for (int i = 0; i < 10; ++i) {
                    const float age =
                        std::fmod(float(adventure_.frame) * 0.045f + float(i) * 0.618034f, 1.0f);
                    const float side = i % 2 == 0 ? 1.0f : -1.0f;
                    const Vector3 spark = {contact.x + side * age * (0.5f + float(i % 3) * 0.3f),
                                           contact.y + age * (1 - age) * 4.5f, contact.z};
                    renderer.AddGlow(spark, {side * 0.5f, 1 - age * 2, 0}, 0.12f, 0.022f,
                                     {4, 0.7f, 0.2f, (1 - age) * fade});
                }
            }

            // 発射時の十字閃光を短くし、持続中はビームの動きを見せる。
            if (adventure_.bossTimer > adventure_.beamDuration - 4) {
                renderer.AddGlow(emitter, {1, 0, 0}, 2.5f, 0.06f, {3, 3, 3, 0.8f});
                renderer.AddGlow(emitter, {0, 1, 0}, 1.5f, 0.04f, {3, 2.5f, 2.5f, 0.8f});
            }
        }
    }

    // 足場の上面と穴の境目を細い光で示し、暗い舞台でも着地点を読めるようにする。
    for (const auto &floor : adventure_.stage.platforms) {
        const Vector3 center = {(floor.left + floor.right) * 0.5f, floor.top - 0.49f, -1.25f};
        const float length = (floor.right - floor.left) * 0.5f;
        renderer.AddGlow(center, {1, 0, 0}, length, 0.06f, {0.14f, 0.25f, 0.45f, 0.35f});
        renderer.AddGlow(center, {1, 0, 0}, length, 0.013f, {0.45f, 0.6f, 0.8f, 0.5f});
    }

    // 遠景の光は弱く、敵の橙の核や攻撃の閃光より目立たせない。
    for (size_t i = 0; i < sceneryModels_.size(); i += 3) {
        const auto &transform = sceneryModels_[i]->GetTransform();
        auto center = transform.translate;
        center.z -= 0.4f;
        renderer.AddGlow(center, {0.15f, 1, 0}, transform.scale.y * 0.6f, 0.025f,
                         Vector4{0.12f, 0.2f, 0.35f, 0.18f});
    }

    for (size_t i = 0; i < adventure_.enemies.size(); ++i) {
        const auto &enemy = adventure_.enemies[i];
        if (!enemy.alive) {
            continue;
        }

        auto center = enemyModels_[i]->GetTransform().translate;
        center.z = -0.7f;
        const float pulse = 0.65f + std::sin(float(adventure_.frame) * 0.05f + float(i)) * 0.15f;
        renderer.AddGlow(center, {1, 0, 0}, 0.24f, 0.13f, {2.5f, 0.55f, 0.13f, pulse});
        renderer.AddArc(center, 0.7f, enemy.kind == 3 ? 1.0f : 0.72f, 4.4f, 0.018f,
                        {1.5f, 0.3f, 0.08f, 0.3f});
        if (enemy.kind == 3) {
            center.y += 0.5f;
            renderer.AddGlow(center, {1, 0, 0}, 0.2f, 0.055f, {2.5f, 1.8f, 1.0f, 0.8f});
        }
    }

    for (const auto &minion : adventure_.minions) {
        if (minion.life > 0) {
            renderer.AddGlow({minion.x, minion.y, -0.7f}, {1, 0, 0}, 0.22f, 0.12f,
                             {2.5f, 0.55f, 0.13f, 0.8f});
        }
    }

    // 床レーザーは地面全体が危険になる予告として、赤い光を点滅させる。
    if (adventure_.bossState == SpearAdventure::BossState::LaserWarning) {
        const float alpha = 0.25f + (std::sin(float(adventure_.frame) * 0.2f) * 0.5f + 0.5f) * 0.4f;
        if (adventure_.scorchedFloorEnabled) {
            renderer.AddScorchedGround({114, -3.48f, 0.6f}, 12, -1, alpha * 0.35f,
                                       adventure_.frame);
        } else {
            renderer.AddGlow({114, -2.9f, -1.3f}, {1, 0, 0}, 12, 0.14f, {2.5f, 0.04f, 0.2f, alpha});
        }
    }

    // ボスは攻撃と同じ橙・銅色の結晶体に揃える。プレイヤーの赤青は使わない。
    if (adventure_.bossState != SpearAdventure::BossState::Dormant && !adventure_.won) {
        if (adventure_.bossState == SpearAdventure::BossState::PhaseChange) {
            const float progress = 1 - float(adventure_.bossTimer) / 90;
            const Vector3 center = {adventure_.bossX, adventure_.bossY, 0.2f};
            renderer.AddArc(center, progress * 2, 1.2f + progress * 3.2f, 5.6f, 0.09f,
                            {2.6f, 0.5f, 0.1f, std::sin(progress * 3.141593f) * 0.75f});
            renderer.AddArc(center, -progress * 2, 0.8f + progress * 2.3f, 5.2f, 0.04f,
                            {2.8f, 1.2f, 0.4f, std::sin(progress * 3.141593f) * 0.7f});
        }

        // Emission follows the Blender mesh itself, including every copper/amber facet.
        // Preserve the existing flash timing so an invulnerable hit remains readable.
        for (size_t index = 0; index < 6; ++index) {
            if (index == 0 && adventure_.bossFlash > 0 && adventure_.bossFlash % 4 < 2) {
                continue;
            }
            const float intensity = index == 1 ? 2.0f : index == 0 ? 1.7f : 1.35f;
            renderer.AddModelEmission(*bossModels_[index]->GetModel(),
                                      bossModels_[index]->GetWorldMatrix(), intensity);
        }

        const float corePulse = adventure_.bossFlash > 0
                                    ? 0.95f
                                    : 0.65f + std::sin(float(adventure_.frame) * 0.08f) * 0.08f;
        const Vector3 nucleus = {adventure_.bossX, adventure_.bossY, -0.95f};
        renderer.AddGlow(nucleus, {1, 0, 0}, 0.28f, 0.22f, {2.8f, 0.7f, 0.14f, corePulse});
        renderer.AddGlow(nucleus, {1, 0, 0}, 0.07f, 0.06f, {3, 2.0f, 1.0f, corePulse * 0.55f});

        const float phase = adventure_.bossVisualAngle * 2.2f;
        const float radius =
            2.25f + adventure_.armorOpening * 0.35f + adventure_.phaseFormBlend * 0.4f;
        const Vector3 center = {adventure_.bossX, adventure_.bossY, 0.8f};
        renderer.AddArc(center, phase, radius, 2.9f, 0.045f, {2.3f, 0.6f, 0.16f, 0.65f});
        renderer.AddArc(center, phase + 3.141593f, radius, 2.9f, 0.045f,
                        {1.9f, 0.3f, 0.08f, 0.65f});
        for (int i = 0; i < 2; ++i) {
            const float angle = phase + float(i) * 3.141593f;
            const Vector3 light = {center.x + std::cos(angle) * radius,
                                   center.y + std::sin(angle) * radius * 0.7f, 0.6f};
            const Vector4 color =
                i == 0 ? Vector4{2.5f, 0.7f, 0.18f, 0.75f} : Vector4{2.1f, 0.3f, 0.08f, 0.75f};
            renderer.AddGlow(light, {1, 0, 0}, 0.35f, 0.15f, color);
        }
    }

    for (const auto &trail : effects_.trails) {
        if (trail.fade <= 0 || trail.count < 2) {
            continue;
        }

        std::array<Vector3, 12> path{};
        for (int i = 0; i < trail.count; ++i) {
            path[i] = {trail.points[i].x, trail.points[i].y, 0.75f};
        }

        const float remaining = float(trail.fade) / 16;
        renderer.AddInterwovenRibbon(std::span(path.data(), trail.count), remaining,
                                     float(16 - trail.fade) * 0.025f);
    }

    for (const auto &shard : effects_.shards) {
        if (shard.life <= 0) {
            continue;
        }

        const float fade = (std::min)(1.0f, float(shard.life) / 12);
        const Vector3 center = {shard.x, shard.y, 0.85f};
        const Vector4 color =
            shard.red ? Vector4{1.8f, 0.08f, 0.3f, fade} : Vector4{0.12f, 0.85f, 1.8f, fade};
        renderer.AddCrystal(center, shard.angle, shard.size * (0.4f + fade * 0.6f), color);
        auto halo = color;
        halo.w *= 0.18f;
        renderer.AddGlow(center, {1, 0, 0}, shard.size, shard.size * 0.5f, halo);
    }

    for (const auto &pulse : effects_.pulses) {
        if (pulse.life <= 0) {
            continue;
        }

        const float fade = float(pulse.life) / 12;
        const float radius = 0.35f + (1 - fade) * (pulse.landing ? 1.4f : 0.8f);
        const Vector3 center = {pulse.x, pulse.y, 0.8f};
        renderer.AddArc(center, 0.4f, radius, 2.3f, 0.025f,
                        {0.1f, 0.7f, 2.2f, fade * fade * 0.65f});
        renderer.AddArc(center, 2.7f, radius, 2.3f, 0.025f,
                        {2.2f, 0.05f, 0.25f, fade * fade * 0.55f});
    }

    for (const auto &flash : effects_.flashes) {
        if (flash.life <= 0) {
            continue;
        }

        const Vector3 center = {flash.x, flash.y, 0.65f};
        const float remaining = float(flash.life) / float(flash.hit ? 13 : 9);
        const float opacity = remaining * remaining;
        if (!flash.hit) {
            // 始動の十字閃光は短く、移動中の本体を隠さない。
            renderer.AddGlow(center, {1, 0, 0}, 3.5f, 0.24f, {0.08f, 0.5f, 2.5f, opacity});
            renderer.AddGlow(center, {1, 0, 0}, 3.3f, 0.032f, {2, 2.5f, 3, opacity});
            renderer.AddGlow(center, {0, 1, 0}, 1.3f, 0.045f, {1.5f, 2, 3, opacity});
            continue;
        }

        const float strength = 1.0f + float(flash.rank) * 0.12f;
        // 撃破の白い芯は最初の1フレームだけ小さく見せ、色の弧と破片へ視線をつなぐ。
        if (flash.rank > 0 && flash.life >= 13) {
            renderer.AddGlow(center, {1, 0, 0}, 1.1f * strength, 0.055f, {1.6f, 2.0f, 2.5f, 0.65f});
            renderer.AddGlow(center, {0, 1, 0}, 0.65f * strength, 0.035f,
                             {1.0f, 1.6f, 2.4f, 0.55f});
        }

        const float radius = (0.7f + (1 - remaining) * 1.4f) * strength;
        renderer.AddArc(center, flash.angle + 0.8f, radius, 2.4f, 0.055f * remaining,
                        {0.2f, 1.0f, 2.4f, opacity * 0.8f});
        renderer.AddArc(center, flash.angle - 1.1f, radius * 0.8f, 2.1f, 0.025f * remaining,
                        {0.35f, 0.8f, 2.5f, opacity});
        if (flash.rank > 0) {
            renderer.AddArc(center, flash.angle + 2.5f, radius * 1.1f, 2.1f, 0.045f * remaining,
                            {2.3f, 0.06f, 0.3f, opacity * 0.65f});
        }

        renderer.AddGlow(center, {std::cos(flash.angle), std::sin(flash.angle), 0}, 1.6f, 0.065f,
                         {0.35f, 1.1f, 2.2f, opacity * 0.7f});
    }

    for (const auto &spark : effects_.sparks) {
        if (spark.life <= 0) {
            continue;
        }

        const float remaining = float(spark.life) / float(spark.duration);
        const Vector4 color = spark.red ? Vector4{2.2f, 0.05f, 0.25f, remaining * remaining}
                                        : Vector4{0.2f, 1.1f, 3.0f, remaining * remaining};
        renderer.AddGlow({spark.x, spark.y, 0.6f}, {spark.vx, spark.vy, 0},
                         0.08f + std::hypot(spark.vx, spark.vy) * 0.8f, 0.035f, color);
    }

    // 既存の撃破・着地粒子も、箱ではなく小さな点として描画する。
    for (size_t i = 0; i < adventure_.particles.size(); ++i) {
        const auto &particle = adventure_.particles[i];
        if (particle.life <= 0) {
            continue;
        }

        const float alpha = (std::min)(1.0f, float(particle.life) / 15);
        const Vector4 color =
            i % 6 == 0 ? Vector4{1.2f, 1.7f, 2.5f, alpha} : Vector4{0.2f, 0.8f, 2.5f, alpha};
        renderer.AddGlow({particle.x, particle.y, 0.8f}, {1, 0, 0}, 0.09f * particle.size,
                         0.055f * particle.size, color);
    }

    renderer.Draw(dashGaugeCamera_);
    return renderer.HasGeometry();
}

void SpearPrototype::PlayStage() {
    try {
        stageDraft_.Validate();
        adventure_.RestartStage(stageDraft_);
        flow_.Reset();
        motion_.settings = editorSettings_.movement;
        effects_ = {};
        landingFlash_ = 0;
        pendingJump_ = false;
        accumulator_ = 0;
        stageEditorOpen_ = false;
        editorOpen_ = false;
        stageStatus_ = "Playing draft. F3 returns to editing; Save writes it to disk.";
    } catch (const std::exception &e) {
        stageStatus_ = std::string("Cannot play: ") + e.what();
    }
}

void SpearPrototype::LoadStage() {
    try {
        const std::filesystem::path path = "Settings/tutorial-stage.json";
        if (!std::filesystem::exists(path)) {
            stageStatus_ = "No saved stage. Using built-in tutorial.";
            return;
        }
        std::ifstream file(path);
        nlohmann::json json;
        file >> json;
        const auto loaded = SpearStage::Parse(json);
        stageDraft_ = loaded;
        adventure_.RestartStage(loaded);
        flow_.Reset();
        effects_ = {};
        stageDirty_ = false;
        stageStatus_ = "Stage loaded.";
    } catch (const std::exception &e) {
        stageStatus_ = std::string("Load failed (current stage kept): ") + e.what();
    }
}

void SpearPrototype::SaveStage() {
    try {
        stageDraft_.Validate();
        const std::filesystem::path path = "Settings/tutorial-stage.json";
        const std::filesystem::path temporary = "Settings/tutorial-stage.json.tmp";
        std::filesystem::create_directories(path.parent_path());
        std::ofstream file(temporary, std::ios::trunc);
        file << stageDraft_.Serialize().dump(2) << '\n';
        file.close();
        if (!file)
            throw std::runtime_error("Could not write stage file");
        if (std::filesystem::exists(path))
            std::filesystem::copy_file(path, "Settings/tutorial-stage.json.bak",
                                       std::filesystem::copy_options::overwrite_existing);
        if (!MoveFileExW(temporary.c_str(), path.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Could not replace stage file");
        stageDirty_ = false;
        stageStatus_ = "Saved. Previous version: Settings/tutorial-stage.json.bak";
    } catch (const std::exception &e) {
        stageStatus_ = std::string("Save failed: ") + e.what();
    }
}

void SpearPrototype::DrawStageEditor() {
#ifdef USE_IMGUI
    ImGui::SetNextWindowSize(ImVec2(1060, 650), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Tutorial stage editor - F3", &stageEditorOpen_)) {
        ImGui::End();
        return;
    }
    ImGui::TextUnformatted(
        "Editing pauses gameplay. Changes stay in the draft until Play from start.");
    if (ImGui::Button("Save"))
        SaveStage();
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        if (stageDirty_)
            ImGui::OpenPopup("Discard draft and load?");
        else
            LoadStage();
    }
    ImGui::SameLine();
    if (ImGui::Button("Built-in tutorial"))
        ImGui::OpenPopup("Replace draft?");
    ImGui::SameLine();
    if (ImGui::Button("Play from start"))
        PlayStage();
    ImGui::SameLine();
    ImGui::TextUnformatted(stageDirty_ ? "* Unsaved changes" : "Saved / unchanged");
    if (ImGui::BeginPopupModal("Discard draft and load?", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Unsaved edits will be lost.");
        if (ImGui::Button("Load saved")) {
            LoadStage();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopupModal("Replace draft?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(
            "Replace this draft with the built-in tutorial? Saved file stays until Save.");
        if (ImGui::Button("Replace")) {
            stageDraft_ = SpearStage{};
            stageDirty_ = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    ImGui::TextWrapped("%s", stageStatus_.c_str());
    ImGui::TextWrapped("File: %s",
                       std::filesystem::absolute("Settings/tutorial-stage.json").string().c_str());
    bool changed = ImGui::Checkbox("Boss battle after tutorial (off: clear at x=98)",
                                   &stageDraft_.bossEnabled);
    ImGui::TextUnformatted("Select / drag on map. Cyan: platforms, red: enemies, green: "
                           "start/checkpoints. Arena is fixed.");
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 size((std::max)(ImGui::GetContentRegionAvail().x, 200.0f), 240);
    ImGui::InvisibleButton("Stage map", size);
    const float sx = size.x / 138.0f, sy = size.y / 25.0f;
    const auto screen = [&](float x, float y) {
        return ImVec2(origin.x + (x + 8) * sx, origin.y + (15 - y) * sy);
    };
    auto *draw = ImGui::GetWindowDrawList();
    draw->PushClipRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), true);
    draw->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y),
                        IM_COL32(20, 27, 38, 255));
    for (int x = 0; x <= 130; x += 10) {
        const auto p = screen(float(x), 15);
        draw->AddLine(p, screen(float(x), -10), IM_COL32(48, 58, 70, 255));
        draw->AddText(p, IM_COL32(160, 170, 180, 255), std::to_string(x).c_str());
    }
    draw->AddRectFilled(screen(98, 12), screen(130, -8), IM_COL32(65, 50, 65, 150));
    draw->AddText(screen(100, 11), IM_COL32(220, 190, 220, 255), "BOSS ARENA");
    for (size_t i = 0; i < 8; ++i) {
        const auto &p = stageDraft_.platforms[i];
        const auto a = screen(p.left, p.top), b = screen(p.right, p.top);
        const bool selected = stageSelectionType_ == 0 && stageSelection_ == int(i);
        draw->AddLine(a, b, selected ? IM_COL32(255, 220, 100, 255) : IM_COL32(80, 200, 230, 255),
                      6);
        draw->AddText(ImVec2(a.x, a.y + 6), IM_COL32(220, 230, 240, 255),
                      ("P" + std::to_string(i)).c_str());
    }
    for (size_t i = 0; i < 7; ++i) {
        const auto &e = stageDraft_.enemies[i];
        const auto p = screen(e.x, e.y);
        const bool selected = stageSelectionType_ == 1 && stageSelection_ == int(i);
        draw->AddCircleFilled(p, selected ? 8.0f : 6.0f,
                              e.enabled ? IM_COL32(255, 110, 100, 255)
                                        : IM_COL32(100, 100, 100, 255));
        draw->AddText(ImVec2(p.x + 8, p.y - 8), IM_COL32(255, 210, 200, 255),
                      ("E" + std::to_string(i)).c_str());
        if (selected && e.enabled)
            draw->AddLine(screen(e.gate, 12), screen(e.gate, -8), IM_COL32(255, 190, 80, 200), 2);
    }
    for (size_t i = 0; i < 6; ++i) {
        const auto &s = stageDraft_.spawns[i];
        const auto p = screen(s.x, s.y);
        draw->AddLine(p, ImVec2(p.x, p.y - 22), IM_COL32(100, 255, 150, 255), 3);
        draw->AddText(ImVec2(p.x + 2, p.y - 30), IM_COL32(100, 255, 150, 255),
                      (i == 0 ? "START" : "C" + std::to_string(i)).c_str());
    }
    draw->PopClipRect();
    if (ImGui::IsItemActivated()) {
        const auto mouse = ImGui::GetIO().MousePos;
        stageMapDragging_ = false;
        float best = 18 * 18.0f;
        const auto pick = [&](float dx, float dy, int type, int index) {
            const float distance = dx * dx + dy * dy;
            if (distance < best) {
                stageMapDragging_ = true;
                best = distance;
                stageSelectionType_ = type;
                stageSelection_ = index;
            }
        };
        for (int i = 0; i < 7; ++i) {
            const auto &p = stageDraft_.platforms[size_t(i)];
            const auto a = screen(p.left, p.top), b = screen(p.right, p.top);
            pick(mouse.x - (std::clamp)(mouse.x, a.x, b.x), mouse.y - a.y, 0, i);
        }
        for (int i = 0; i < 7; ++i) {
            const auto &e = stageDraft_.enemies[size_t(i)];
            const auto p = screen(e.x, e.y);
            pick(mouse.x - p.x, mouse.y - p.y, 1, i);
        }
        for (int i = 0; i < 6; ++i) {
            const auto &s = stageDraft_.spawns[size_t(i)];
            const auto p = screen(s.x, s.y);
            pick(mouse.x - p.x, mouse.y - (p.y - 16), 2, i);
        }
    }
    if (stageMapDragging_ && ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        const auto d = ImGui::GetIO().MouseDelta;
        const float dx = d.x / sx, dy = -d.y / sy;
        if (stageSelectionType_ == 0 && stageSelection_ < 7) {
            auto &p = stageDraft_.platforms[size_t(stageSelection_)];
            const float move = (std::clamp)(dx, -8 - p.left, 98 - p.right);
            const float rise = (std::clamp)(dy, -8 - p.top, 10 - p.top);
            // Keep checkpoint markers on a moved supporting platform.
            for (auto &s : stageDraft_.spawns)
                if (s.x >= p.left && s.x <= p.right && std::abs(s.y - p.top) < 0.01f) {
                    s.x += move;
                    s.y += rise;
                }
            p.left += move;
            p.right += move;
            p.top += rise;
            changed |= move != 0 || rise != 0;
        } else if (stageSelectionType_ == 1) {
            auto &e = stageDraft_.enemies[size_t(stageSelection_)];
            e.x = (std::clamp)(e.x + dx, -6.0f, 96.0f);
            e.y = (std::clamp)(e.y + dy, -7.0f, 11.0f);
            changed |= dx != 0 || dy != 0;
        } else if (stageSelectionType_ == 2) {
            auto &s = stageDraft_.spawns[size_t(stageSelection_)];
            s.x = (std::clamp)(s.x + dx, -7.0f, 96.0f);
            s.y = (std::clamp)(s.y + dy, -8.0f, 10.0f);
            for (const auto &p : stageDraft_.platforms)
                if (s.x >= p.left + 0.5f && s.x <= p.right - 0.5f && std::abs(s.y - p.top) < 0.5f)
                    s.y = p.top;
            changed |= dx != 0 || dy != 0;
        }
    }
    const char *types[] = {"Platform", "Enemy", "Start / checkpoint"};
    if (ImGui::Combo("Selection", &stageSelectionType_, types, 3))
        stageSelection_ = 0;
    ImGui::SliderInt("Slot", &stageSelection_, 0,
                     stageSelectionType_ == 0   ? 7
                     : stageSelectionType_ == 1 ? 6
                                                : 5);
    const auto edit = [&](const char *label, float &v, float lo, float hi) {
        return ImGui::DragFloat(label, &v, 0.05f, lo, hi, "%.2f", ImGuiSliderFlags_AlwaysClamp);
    };
    if (stageSelectionType_ == 0) {
        auto &p = stageDraft_.platforms[size_t(stageSelection_)];
        if (stageSelection_ == 7)
            ImGui::TextUnformatted("Boss arena floor: fixed at [98,130], y=-3.");
        else {
            changed |= edit("Left", p.left, -8, p.right - 1);
            changed |= edit("Right", p.right, p.left + 1, 98);
            changed |= edit("Top", p.top, -8, 10);
        }
    } else if (stageSelectionType_ == 1) {
        auto &e = stageDraft_.enemies[size_t(stageSelection_)];
        changed |= ImGui::Checkbox("Enabled", &e.enabled);
        changed |= edit("X", e.x, -6, 96);
        changed |= edit("Y", e.y, -7, 11);
        const char *kinds[] = {"Normal", "Air chain", "Up thrust", "Down thrust"};
        changed |= ImGui::Combo("Kind", &e.kind, kinds, 4);
        changed |= edit("Gate X (opens when defeated)", e.gate, -5, 98);
        auto &lesson = stageDraft_.lessons[size_t(stageSelection_)];
        char buffer[256]{};
        std::copy(lesson.begin(), lesson.end(), buffer);
        if (ImGui::InputText("Lesson (English)", buffer, sizeof(buffer))) {
            lesson = buffer;
            changed = true;
        }
    } else {
        auto &s = stageDraft_.spawns[size_t(stageSelection_)];
        changed |= edit("Spawn X", s.x, -7, 96);
        changed |= edit("Spawn Y (platform top)", s.y, -8, 10);
        ImGui::TextUnformatted("Slot 0: start. Slots 1-5: checkpoints in ascending X order.");
    }
    stageDirty_ |= changed;
    try {
        stageDraft_.Validate();
        ImGui::TextColored(ImVec4(0.5f, 1, 0.6f, 1), "Ready to save / play");
    } catch (const std::exception &e) {
        ImGui::TextWrapped("Fix before save / play: %s", e.what());
    }
    ImGui::End();
#endif
}

void SpearPrototype::RestartRun() {
    restartTransition_.Begin(true);
    pendingJump_ = false;
    jumpHeld_ = false;
    accumulator_ = 0;
    lastUpdate_ = std::chrono::steady_clock::now();
}

void SpearPrototype::ApplyRestart(bool fullRun) {
    if (fullRun) {
        adventure_.RestartStage(adventure_.stage);
        flow_.Reset();
    } else {
        ++adventure_.deaths;
        adventure_.Respawn();
    }
    motion_.settings = editorSettings_.movement;
    effects_ = {};
    pendingJump_ = false;
    jumpHeld_ = false;
    landingFlash_ = 0;
    UpdateObjects();
}

