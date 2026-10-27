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
    void SetPaused(bool paused);
    void RetryFromPause();
    bool IsPaused() const { return pause_.active; }
    std::string PauseSimulationStamp() const;
    void OpenUiEditor(const char *page = "title");
    void SetUiEditorTestSavePath() { uiSaveOverride_ = "Settings/ui-editor-smoke.json"; }
    bool IsResultReady() const { return flow_.CanReplay() && !restartTransition_.Active(); }
    bool DrawEffects(EffectRenderer &renderer);
    void SetFixedPreviewStep() {
        fixedPreviewStep_ = true;
    }

    void BeginStageEditorSmokeTest() {
        stageDraft_ = SpearStage{};stageUndo_.clear();stageRedo_.clear();
        stageSaveOverride_ = "Settings/stage-editor-smoke.json";
        stageMapX_=61;stageMapY_=2.5f;stageMapWidth_=138;stageTool_=0;
        stageSelectionType_=stageSelection_=0;stageEditorOpen_=true;editorOpen_=false;
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
        adventure_.bossState = role == 0   ? SpearAdventure::BossState::BeamWarning
                               : role == 1 ? SpearAdventure::BossState::SummonWarning
                                           : SpearAdventure::BossState::BeamWarning;
        adventure_.bossTimer = 45;
        adventure_.beamWarningDuration = 45;
        adventure_.bossTargetX = adventure_.stage.ArenaX(118);
        adventure_.bossTargetY = 4;
        adventure_.beamAimAngle = std::atan2(motion_.y - 4, motion_.x - adventure_.stage.ArenaX(118));
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
                adventure_.bossTargetX=adventure_.bossX=adventure_.stage.ArenaX(114);
                adventure_.bossTargetY=adventure_.bossY=4.8f;
                adventure_.bossTimer=70;
                adventure_.novaGapAngle=std::atan2(motion_.y-adventure_.bossY,motion_.x-adventure_.bossX);
                adventure_.novaWave=0;
            }
            if(role==8) {
                adventure_.secondPhase=false;adventure_.phaseFormBlend=0;
                adventure_.bossHp=SpearAdventure::bossMaxHp;
                adventure_.bossState=SpearAdventure::BossState::Recovery;adventure_.bossTimer=150;
                adventure_.bossX=adventure_.bossTargetX=adventure_.stage.ArenaX(114);
                adventure_.bossY=adventure_.bossTargetY=0;
                motion_.x=adventure_.stage.ArenaX(114);motion_.y=2.7f;motion_.grounded=false;motion_.dashFrames=0;motion_.charges=3;
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
    void PreviewJumpCount(int charges) {
        flow_.Reset();adventure_.RestartStage(adventure_.stage);effects_={};
        motion_.x=12;motion_.y=1;motion_.grounded=false;motion_.charges=(std::clamp)(charges,0,3);
        adventure_.cameraX=14;adventure_.invincibility=1000;
        jumpUseFlash_=0;jumpSpentSlot_=-1;
    }

    void AudioSmokeAction(int action);
    std::string AudioStatus() const;
    void PreviewFinish(int point) {
        flow_.Reset();adventure_.RestartStage(adventure_.stage);effects_={};
        adventure_.invincibility=10000;
        if(point>=4) {adventure_.StartBossPractice();adventure_.bossTimer=1;adventure_.invincibility=10000;}
        else {
            static constexpr float positions[]={2,15,30,83};
            motion_.x=positions[point];motion_.y=-3;motion_.grounded=true;
            for(auto &enemy:adventure_.enemies) if(enemy.x<motion_.x-1) enemy.alive=false;
            adventure_.cameraX=motion_.x+2.5f;adventure_.cameraY=1.5f;adventure_.cameraDistance=32;
        }
        UpdateObjects();
    }
    void RequestFinishPhase() {adventure_.bossHp=SpearAdventure::phaseThreshold;}
    void PreviewAirRoute() {
        flow_.Reset();adventure_.RestartStage(adventure_.stage);effects_={};
        for(auto& enemy:adventure_.enemies) if(enemy.x<31) enemy.alive=false;
        motion_.x=30;motion_.y=-3;motion_.grounded=true;adventure_.checkpoint=2;
        jumpHeld_=true;
        for(int f=0;f<18;++f) FixedUpdate(0,0,f==0);
        for(int f=0;f<11;++f) FixedUpdate(1,0,f==0);
        adventure_.cameraX=39;adventure_.cameraY=1.5f;adventure_.cameraDistance=32;UpdateObjects();
    }
    void PreviewAirChain(bool strike) {
        flow_.Reset();adventure_.StartBossPractice();effects_={};
        adventure_.frame=0;adventure_.invincibility=1000;
        adventure_.bossState=SpearAdventure::BossState::Recovery;adventure_.bossTimer=1000;
        adventure_.bossX=adventure_.bossTargetX=adventure_.stage.ArenaX(124);
        adventure_.bossY=adventure_.bossTargetY=-.4f;
        motion_.x=adventure_.stage.ArenaX(103);motion_.y=-.4f;motion_.grounded=false;motion_.charges=1;
        adventure_.minions={};
        for(int i=0;i<3;++i) adventure_.minions[size_t(i)]={adventure_.stage.ArenaX(107.5f+float(i)*6),-.4f,0,0,420,-1};
        for(int f=0;f<(strike?44:22);++f) FixedUpdate(1,0,f%11==0);
        adventure_.cameraX=adventure_.stage.ArenaX(114);adventure_.cameraY=2;adventure_.cameraDistance=37;
        UpdateObjects();
    }

    void PreviewTutorialRefill() {
        flow_.Reset();adventure_.RestartStage(adventure_.stage);effects_={};
        for(const auto &enemy:adventure_.enemies) if(enemy.alive) {
            motion_.x=enemy.x-2;motion_.y=enemy.y;motion_.grounded=false;motion_.charges=1;
            adventure_.cameraX=enemy.x-2;adventure_.cameraY=1.5f;
            FixedUpdate(1,0,true);return;
        }
    }

    void PreviewPierce() {
        flow_.Reset();
        adventure_.StartBossPractice();
        adventure_.invincibility = 0;
        adventure_.formationAxis = 0;
        adventure_.bossTimer = 300;
        motion_.x = adventure_.stage.ArenaX(105);
        motion_.y = 0.6f;
        motion_.grounded = false;
        adventure_.formationX = adventure_.stage.ArenaX(109);
        for (size_t i = 0; i < adventure_.minions.size(); ++i) {
            adventure_.minions[i] = {adventure_.stage.ArenaX(106.7f) + float(i) * 1.45f, 0.6f, 0, 0, 120, -1};
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
        adventure_.dashStartX = adventure_.stage.ArenaX(110);
        adventure_.dashStartY = 0;
        adventure_.dashTargetX = adventure_.stage.ArenaX(116);
        adventure_.dashTargetY = 0;
        adventure_.bossX = adventure_.stage.ArenaX(113);
        adventure_.bossY = 0;
        motion_.x = adventure_.stage.ArenaX(113);
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
    void DrawAmbientEffects(EffectRenderer &renderer);
    float ambientTime_ = 0;
    void DrawBossOverhaulEffects(EffectRenderer &renderer);
    void UpdateObjects();
    void FixedUpdate(float directionX, float directionY, bool pressed);
    void DrawEditor();
    void UpdateEffectPreview();
    void DrawStageEditor();
    void RememberStageEdit();
    void LoadStage();
    void SaveStage();
    void PlayStage();
    void LoadSettings();
    void SaveSettings();
    void InitializeAdventureModels();
    void UpdateAdventureModels();
    void UpdateSceneLighting();
    void PlayEffect(size_t index);
    enum class ExtraCue : size_t { Landing, StrongHit, Checkpoint, Fan, Beam, Select, Confirm, Back, Count };
    void InitializeSelectedAudio();
    void FinalizeSelectedAudio();
    void UpdateGameAudio(float elapsed);
    void PauseGameAudio(bool paused);
    void StopGameSoundLayers();
    void PlayExtra(ExtraCue cue);
    static constexpr size_t extraCueCount=size_t(ExtraCue::Count);
    std::array<SoundData,extraCueCount> extraSounds_{};
    std::array<Audio::VoiceId,extraCueCount> extraVoices_{};
    std::array<int,extraCueCount> extraPlayCounts_{};
    std::array<SoundData,2> musicSounds_{};
    std::array<Audio::VoiceId,2> musicVoices_{};
    std::array<float,2> musicGains_{};
    std::array<int,2> musicStarts_{};
    std::array<int,8> legacyPlayCounts_{};
    bool selectedAudioReady_=false, gameAudioPaused_=false;

    bool HandlePause(bool toggle, bool confirm, bool cancel, int navigation,
                     bool click, float mouseX, float mouseY);
    void EndPause(bool resumeAudio);
    int PauseChoiceAt(float x,float y) const;
    SpearPauseState pause_;
    unsigned short previousPadButtons_=0;
    int previousPauseStick_=0;
    std::array<float,2> pauseMouse_{};
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
    bool effectPreview_ = true;
    int effectPreviewFrame_ = 0;
    SpearEffects previewEffects_;
    SpearStage stageDraft_;
    bool stageEditorOpen_ = false;
    bool stageDirty_ = false;
    bool stageMapDragging_ = false;
    int stageSelectionType_ = 0;
    int stageSelection_ = 0;
    std::string stageStatus_;
    std::string stageSaveOverride_;
    std::vector<SpearStage> stageUndo_, stageRedo_;
    int stageTool_ = 0;
    bool stageSnap_ = true, stageDragRecorded_ = false;
    float stageGrid_ = .5f, stageMapX_ = 61, stageMapY_ = 2.5f, stageMapWidth_ = 138;
    float stageDragX_ = 0, stageDragY_ = 0;
    int stageDragHandle_ = 0;
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
    int jumpUseFlash_ = 0, jumpSpentSlot_ = -1;
    int bossImpactSoundCooldown_ = 0;
    std::vector<std::unique_ptr<Object3d>> platformModels_;
    std::array<std::unique_ptr<Object3d>, 18> sceneryModels_;
    std::vector<std::unique_ptr<Object3d>> enemyModels_;
    std::array<std::unique_ptr<Object3d>, 8> bossModels_;
    std::array<std::unique_ptr<Object3d>, 12> hazardModels_;
    std::array<std::unique_ptr<Object3d>, 48> particleModels_;
    std::array<std::unique_ptr<Object3d>, 8> impactModels_;
    std::array<std::unique_ptr<Object3d>, 4> minionModels_;
    std::unique_ptr<Object3d> gateModel_;
    std::unique_ptr<Object3d> warningModel_;
    std::unique_ptr<Object3d> backdropModel_;
    std::unique_ptr<Object3d> laserCoreModel_;
    std::array<SoundData, 8> effectSounds_{};
    Audio::VoiceId refillVoice_ = 0;
    Audio::VoiceId effectVoice_ = 0;
    std::array<float, 2> victoryBlurCenter_{0.5f, 0.5f};
    std::unique_ptr<Object3d> trailCoreModel_;
    std::array<float, 2> feedbackScreenPoint_{640, 360};
    std::array<std::unique_ptr<Object3d>, 4> dashGaugeModels_;
    ViewProjection dashGaugeCamera_;
    std::array<float, 2> dashGaugeScreenPoint_{640, 360};
};


