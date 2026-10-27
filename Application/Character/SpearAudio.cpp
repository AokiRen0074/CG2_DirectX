#include "SpearPrototype.h"
#include <algorithm>
#include <cmath>

void SpearPrototype::InitializeSelectedAudio() {
  auto *audio = Audio::GetInstance();
  constexpr const char *effects[] = {
      "landing.wav", "strong_hit.wav", "checkpoint.wav", "fan_shot.wav",
      "laser.wav",   "ui_select.wav",  "ui_confirm.wav", "ui_back.wav"};
  constexpr const char *music[] = {"title_outer_space.wav",
                                   "battle_electric_stream.wav"};
  for (size_t i = 0; i < extraSounds_.size(); ++i)
    extraSounds_[i] = audio->SoundLoadWave(
        (std::string("Resources/Audio/Selected/") + effects[i]).c_str());
  for (size_t i = 0; i < musicSounds_.size(); ++i)
    musicSounds_[i] = audio->SoundLoadWave(
        (std::string("Resources/Audio/Selected/") + music[i]).c_str());
  selectedAudioReady_ = true;
}

void SpearPrototype::FinalizeSelectedAudio() {
  auto *audio = Audio::GetInstance();
  for (auto id : extraVoices_)
    audio->StopVoice(id);
  for (auto id : musicVoices_)
    audio->StopVoice(id);
  for (auto &sound : extraSounds_)
    audio->SoundUnload(&sound);
  for (auto &sound : musicSounds_)
    audio->SoundUnload(&sound);
  selectedAudioReady_ = false;
}

void SpearPrototype::PlayExtra(ExtraCue cue) {
  if (!selectedAudioReady_)
    return;
  const size_t index = size_t(cue);
  constexpr float gains[] = {.12f, .22f, .22f, .15f, .22f, .10f, .16f, .12f};
  auto *audio = Audio::GetInstance();
  audio->StopVoice(extraVoices_[index]);
  extraVoices_[index] = audio->PlayVoice(extraSounds_[index]);
  audio->SetVoiceVolume(extraVoices_[index], gains[index]);
  ++extraPlayCounts_[index];
}

void SpearPrototype::UpdateGameAudio(float elapsed) {
  if (!selectedAudioReady_ || gameAudioPaused_)
    return;
  auto *audio = Audio::GetInstance();
  const bool boss = adventure_.bossState != SpearAdventure::BossState::Dormant;
  const float titleAlpha = SpearGameFlow::TitleAlpha(
      adventure_.cameraX, adventure_.stage.spawns[0].x + 5);
  const std::array<float, 2> targets = {
      (!boss && !adventure_.won) ? (.07f + titleAlpha * .05f) : 0.0f,
      (boss && !adventure_.won) ? .11f : 0.0f};
  const float master = 1 - restartTransition_.Alpha();
  for (size_t i = 0; i < musicVoices_.size(); ++i) {
    if (targets[i] > .0001f &&
        !audio->GetVoiceSnapshot(musicVoices_[i]).active) {
      musicVoices_[i] = audio->PlayVoice(musicSounds_[i], true);
      audio->SetVoiceVolume(musicVoices_[i], 0);
      ++musicStarts_[i];
    }
    musicGains_[i] += (std::clamp)(targets[i] - musicGains_[i], -elapsed * .20f,
                                   elapsed * .20f);
    audio->SetVoiceVolume(musicVoices_[i], musicGains_[i] * master);
    if (targets[i] <= .0001f && musicGains_[i] <= .0001f) {
      audio->StopVoice(musicVoices_[i]);
      musicVoices_[i] = 0;
      musicGains_[i] = 0;
    }
  }
}

void SpearPrototype::PauseGameAudio(bool paused) {
  gameAudioPaused_ = paused;
  auto *audio = Audio::GetInstance();
  audio->PauseVoice(effectVoice_, paused);
  audio->PauseVoice(refillVoice_, paused);
  for (size_t i = 0; i < size_t(ExtraCue::Select); ++i)
    audio->PauseVoice(extraVoices_[i], paused);
  for (auto id : musicVoices_)
    audio->PauseVoice(id, paused);
  // Menu feedback remains audible while the world and music are paused.
}

void SpearPrototype::StopGameSoundLayers() {
  auto *audio = Audio::GetInstance();
  audio->StopVoice(effectVoice_);
  effectVoice_ = 0;
  audio->StopVoice(refillVoice_);
  refillVoice_ = 0;
  for (size_t i = 0; i < size_t(ExtraCue::Select); ++i) {
    audio->StopVoice(extraVoices_[i]);
    extraVoices_[i] = 0;
  }
}

std::string SpearPrototype::AudioStatus() const {
  nlohmann::json j{{"selected", extraPlayCounts_},
                   {"musicStarts", musicStarts_},
                   {"gains", musicGains_},
                   {"paused", gameAudioPaused_},
                   {"legacy", legacyPlayCounts_}};
  j["music"] = nlohmann::json::array();
  for (auto id : musicVoices_) {
    const auto state = Audio::GetInstance()->GetVoiceSnapshot(id);
    j["music"].push_back({{"active", state.active},
                          {"paused", state.paused},
                          {"samples", state.samplesPlayed}});
  }
  return j.dump();
}

void SpearPrototype::AudioSmokeAction(int action) {
  // Trigger production collision and UI paths, rather than playing the cues
  // directly.
  if (action == 0) {
    motion_.x = adventure_.stage.spawns[0].x;
    motion_.y = -2.9f;
    motion_.grounded = false;
    motion_.velocityY = -.12f;
    motion_.velocityX = 0;
    motion_.dashFrames = 0;
    FixedUpdate(0, 0, false);
  } else if (action == 1) {
    motion_.x = adventure_.stage.spawns[1].x - .01f;
    motion_.y = adventure_.stage.spawns[1].y;
    motion_.grounded = true;
    motion_.velocityX = .14f;
    adventure_.checkpoint = 0;
    FixedUpdate(1, 0, false);
  } else if (action == 2)
    PreviewBossAttack(8);
  else if (action == 3)
    PreviewBossAttack(3);
  else if (action == 4)
    PreviewBossAttack(2);
  else if (action == 5)
    SetPaused(true);
  else if (action == 6)
    HandlePause(false, false, false, 1, false, pauseMouse_[0], pauseMouse_[1]);
  else if (action == 7)
    HandlePause(false, false, true, 0, false, pauseMouse_[0], pauseMouse_[1]);
  else if (action == 8)
    HandlePause(false, true, false, 0, false, pauseMouse_[0], pauseMouse_[1]);
  else if (action == 9)
    RestartRun();
}
