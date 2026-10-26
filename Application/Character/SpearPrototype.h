#pragma once
#include "3D/Object3d.h"
#include "SpearMotion.h"
#include "SpearEditorSettings.h"
#include "SpearAdventure.h"
#include "Audio/Audio.h"
#include "SpearEffects.h"
#include "3D/EffectRenderer.h"
#include "SpearGameFlow.h"
#include "SpearScreenRenderer.h"
#include "SpearUiLayout.h"
#include <array>
#include <chrono>
#include <memory>

// ボス戦に進む前に、槍の移動と攻撃を試すためのステージ。
class SpearPrototype {
public:
    void Initialize(DirectXCommon *graphics);
    ~SpearPrototype();
    void Update();
    void Draw(const ViewProjection &camera);
    void DrawPresentation();
    void RestartRun();
    void OpenUiEditor(const char *page = "title");
    void SetUiEditorTestSavePath() { uiSaveOverride_ = "Settings/ui-editor-smoke.json"; }
    bool IsResultReady() const { return flow_.CanReplay() && !restartTransition_.Active(); }
    bool DrawEffects(EffectRenderer &renderer);
    void SetFixedPreviewStep() {
        fixedPreviewStep_ = true;
    }

    void OpenStageEditor() { stageEditorOpen_ = true; editorOpen_ = false; }

    void OpenEditor() {
        editorOpen_ = true;
    }

    void OpenBossPractice() {
        flow_.Reset();
        adventure_.StartBossPractice();
        effects_ = {};
    }

    float VictoryBlurStrength() const {
        const float remaining = float(adventure_.defeatFrames) / 42.0f;
        return remaining * remaining * 0.14f;
    }

    std::array<float, 2> VictoryBlurCenter() const {
        return victoryBlurCenter_;
    }

    void PreviewBossAttack(int role) {
        flow_.Reset();
        adventure_.StartBossPractice();
        adventure_.bossState = role == 0   ? SpearAdventure::BossState::LaserWarning
                               : role == 1 ? SpearAdventure::BossState::SummonWarning
                                           : SpearAdventure::BossState::BeamWarning;
        adventure_.bossTimer = 45;
        adventure_.beamWarningDuration = 45;
        adventure_.bossTargetX = 118;
        adventure_.bossTargetY = 4;
        adventure_.beamAimAngle = std::atan2(motion_.y - 4, motion_.x - 118);
        if (role >= 3) {
            adventure_.secondPhase = role >= 4;
            adventure_.bossHp =
                role >= 4 ? SpearAdventure::phaseThreshold : SpearAdventure::bossMaxHp;
            adventure_.phaseFormBlend = role >= 4 ? 1.0f : 0.0f;
            adventure_.bossState = role == 3   ? SpearAdventure::BossState::FanWarning
                                   : role == 4 ? SpearAdventure::BossState::RainWarning
                                               : SpearAdventure::BossState::DashWarning;
            adventure_.bossTimer = 50;
            adventure_.fanAimAngle =
                std::atan2(motion_.y - adventure_.bossY, motion_.x - adventure_.bossX);
            adventure_.rainSafeLane = 1;
            adventure_.dashTargetX = motion_.x;
            adventure_.dashTargetY = motion_.y;
            adventure_.dashComboRemaining = role == 6 ? 2 : 0;
            if (role == 7) {
                adventure_.bossState=SpearAdventure::BossState::NovaWarning;
                adventure_.bossTargetX=adventure_.bossX=114;
                adventure_.bossTargetY=adventure_.bossY=4.8f;
                adventure_.bossTimer=70;
                adventure_.novaGapAngle=std::atan2(motion_.y-adventure_.bossY,motion_.x-adventure_.bossX);
                adventure_.novaWave=0;
            }
            if(role==8) {
                adventure_.secondPhase=false;adventure_.phaseFormBlend=0;
                adventure_.bossHp=SpearAdventure::bossMaxHp;
                adventure_.bossState=SpearAdventure::BossState::Recovery;adventure_.bossTimer=150;
                adventure_.bossX=adventure_.bossTargetX=114;
                adventure_.bossY=adventure_.bossTargetY=0;
                motion_.x=114;motion_.y=2.7f;motion_.grounded=false;motion_.dashFrames=0;motion_.charges=3;
                adventure_.counterFrames=90;
                FixedUpdate(0,-1,true);
            }
            if (role == 5) {
                adventure_.phaseFormBlend = 0;
                adventure_.BeginSecondPhase();
            }
        }
    }

    void PreviewBossDefeat() {
        adventure_.bossState = SpearAdventure::BossState::Recovery;
        adventure_.bossTimer = 100;
        adventure_.bossHp = 4;
        motion_.x = adventure_.bossX;
        motion_.y = adventure_.bossY + 2.7f;
        motion_.grounded = false;
        motion_.dashFrames = 0;
        motion_.charges = 3;
        FixedUpdate(0, -1, true);
    }

    // プレビューも通常の攻撃・回避判定を通して確認する。
    void PreviewPierce() {
        flow_.Reset();
        adventure_.StartBossPractice();
        adventure_.invincibility = 0;
        adventure_.formationAxis = 0;
        adventure_.bossTimer = 300;
        motion_.x = 105;
        motion_.y = 0.6f;
        motion_.grounded = false;
        adventure_.formationX = 109;
        for (size_t i = 0; i < adventure_.minions.size(); ++i) {
            adventure_.minions[i] = {106.7f + float(i) * 1.45f, 0.6f, 0, 0, 120, -1};
        }

        FixedUpdate(1, 0, true);
    }

    void PreviewDodge() {
        flow_.Reset();
        adventure_.StartBossPractice();
        adventure_.invincibility = 0;
        adventure_.bossState = SpearAdventure::BossState::Dash;
        adventure_.bossTimer = 18;
        adventure_.dashDuration = 34;
        adventure_.dashStartX = 110;
        adventure_.dashStartY = 0;
        adventure_.dashTargetX = 116;
        adventure_.dashTargetY = 0;
        adventure_.bossX = 113;
        adventure_.bossY = 0;
        motion_.x = 113;
        motion_.y = 1.7f;
        motion_.grounded = false;
        FixedUpdate(0, 1, true);
    }

private:
    void ApplyRestart(bool fullRun);
    void PrepareUiAssets(const SpearUiLayout &layout);
    void LoadUiLayout(bool fromSource);
    void DrawUiEditor();
    void RememberUiEdit();
    SpearUiContext UiContext(bool preview) const;
    bool UiReplayHovered(float x, float y) const;
    void DrawBossOverhaulEffects(EffectRenderer &renderer);
    void UpdateObjects();
    void FixedUpdate(float directionX, float directionY, bool pressed);
    void DrawEditor();
    void DrawStageEditor();
    void LoadStage();
    void SaveStage();
    void PlayStage();
    void LoadSettings();
    void SaveSettings();
    void InitializeAdventureModels();
    void UpdateAdventureModels();
    void UpdateSceneLighting();
    void PlayEffect(size_t index);

    SpearAdventure adventure_;
    SpearEffects effects_;
    SpearGameFlow flow_;
    SpearRestartTransition restartTransition_;
    SpearScreenRenderer screen_;
    SpearUiLayout uiLayout_;
    bool uiEditorOpen_ = false, uiDirty_ = false, uiDragging_ = false, uiResizing_ = false, uiCanvasFocused_ = false;
    bool uiDragRecorded_ = false;
    std::string uiSaveOverride_;
    int uiSelection_ = -1, uiNewId_ = 0;
    std::string uiPreviewPage_ = "title", uiStatus_;
    std::vector<std::string> uiAssets_;
    std::vector<SpearUiLayout> uiUndo_, uiRedo_;
    std::array<float,2> uiDragStart_{};
    SpearUiElement uiDragOriginal_;
    bool debugHudOpen_ = false;
    bool previousMouseLeft_ = false;
    bool replayHovered_ = false;
    SpearMotion &motion_ = adventure_.player;
    SpearEditorSettings editorSettings_;
    bool editorOpen_ = false;
    SpearStage stageDraft_;
    bool stageEditorOpen_ = false;
    bool stageDirty_ = false;
    bool stageMapDragging_ = false;
    int stageSelectionType_ = 0;
    int stageSelection_ = 0;
    std::string stageStatus_;
    int selectedObject_ = 0;
    std::string editorStatus_;
    std::array<std::unique_ptr<Object3d>, 8> objects_;
    std::array<std::unique_ptr<Object3d>, 4> playerArmor_;
    std::chrono::steady_clock::time_point lastUpdate_;
    float accumulator_ = 0.0f;
    bool pendingJump_ = false;
    bool fixedPreviewStep_ = false;
    bool previousPadJump_ = false;
    bool jumpHeld_ = false;
    int landingFlash_ = 0;
    int bossImpactSoundCooldown_ = 0;
    std::array<std::unique_ptr<Object3d>, 8> platformModels_;
    std::array<std::unique_ptr<Object3d>, 18> sceneryModels_;
    std::array<std::unique_ptr<Object3d>, 7> enemyModels_;
    std::array<std::unique_ptr<Object3d>, 8> bossModels_;
    std::array<std::unique_ptr<Object3d>, 12> hazardModels_;
    std::array<std::unique_ptr<Object3d>, 48> particleModels_;
    std::array<std::unique_ptr<Object3d>, 8> impactModels_;
    std::array<std::unique_ptr<Object3d>, 4> minionModels_;
    std::unique_ptr<Object3d> gateModel_;
    std::unique_ptr<Object3d> warningModel_;
    std::unique_ptr<Object3d> backdropModel_;
    std::unique_ptr<Object3d> laserCoreModel_;
    std::array<SoundData, 6> effectSounds_{};
    IXAudio2SourceVoice *effectVoice_ = nullptr;
    std::array<float, 2> victoryBlurCenter_{0.5f, 0.5f};
    std::unique_ptr<Object3d> trailCoreModel_;
    std::array<float, 2> feedbackScreenPoint_{640, 360};
    std::array<std::unique_ptr<Object3d>, 4> dashGaugeModels_;
    ViewProjection dashGaugeCamera_;
    std::array<float, 2> dashGaugeScreenPoint_{640, 360};
};


