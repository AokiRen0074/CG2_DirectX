#include "Base/WindowApp.h"
#include "SpearPrototype.h"
#include <Windows.h>
#include <cmath>

void SpearPrototype::SetPaused(bool paused) {
  if (!paused) {
    if (pause_.active)
      EndPause(true);
    return;
  }
  if (pause_.active || adventure_.won || flow_.Finished() ||
      restartTransition_.Active() || editorOpen_ || stageEditorOpen_ ||
      uiEditorOpen_)
    return;
  pause_.Open();
  pendingJump_ = jumpHeld_ = false;
  accumulator_ = 0;
  lastUpdate_ = std::chrono::steady_clock::now();
  POINT cursor{};
  GetCursorPos(&cursor);
  ScreenToClient(WindowApp::GetInstance()->GetHwnd(), &cursor);
  pauseMouse_ = {float(cursor.x), float(cursor.y)};
  PauseGameAudio(true);
}

void SpearPrototype::EndPause(bool resumeAudio) {
  pause_.Close();
  pendingJump_ = jumpHeld_ = false;
  accumulator_ = 0;
  lastUpdate_ = std::chrono::steady_clock::now();
  PauseGameAudio(false);
  if(!resumeAudio) StopGameSoundLayers();
}

void SpearPrototype::RetryFromPause() {
  if (!pause_.active)
    return;
  EndPause(false);
  restartTransition_.Begin(false);
}

int SpearPrototype::PauseChoiceAt(float x, float y) const {
  for (const auto &draw : uiLayout_.Resolve(UiContext(false))) {
    if (!draw.Contains(x, y))
      continue;
    const auto &action = uiLayout_.elements[draw.element].action;
    if (action == "pauseResume")
      return 0;
    if (action == "pauseRetry")
      return 1;
  }
  return -1;
}

bool SpearPrototype::HandlePause(bool toggle, bool confirm, bool cancel,
                                 int navigation, bool click, float mouseX,
                                 float mouseY) {
  if (toggle) {
    const bool wasPaused = pause_.active;
    if(wasPaused) PlayExtra(ExtraCue::Back);
    SetPaused(!wasPaused);
    return wasPaused || pause_.active;
  }
  if (!pause_.active)
    return false;
  const int previousSelection=pause_.selection;
  pause_.Move(navigation);
  const bool moved = std::abs(mouseX - pauseMouse_[0]) > 1 ||
                     std::abs(mouseY - pauseMouse_[1]) > 1;
  pauseMouse_ = {mouseX, mouseY};
  if (moved || click) {
    const int hovered = PauseChoiceAt(mouseX, mouseY);
    if (hovered >= 0) {
      pause_.selection = hovered;
      if (click)
        confirm = true;
    }
  }
  if(pause_.selection!=previousSelection) PlayExtra(ExtraCue::Select);
  if (cancel) {
    PlayExtra(ExtraCue::Back);
    SetPaused(false);
  }
  else if (confirm) {
    PlayExtra(ExtraCue::Confirm);
    if (pause_.Confirm() == SpearPauseState::Action::Retry)
      RetryFromPause();
    else
      SetPaused(false);
  }
  return true; // Consume the resume frame too; never pass its input to
               // gameplay.
}

std::string SpearPrototype::PauseSimulationStamp() const {
  nlohmann::json j{
      {"frame", adventure_.frame},
      {"bossBlend", {adventure_.bossChargeBlend,adventure_.bossNovaBlend,adventure_.bossHitBlend}},
      {"bossPhasePending",adventure_.phaseChangePending},
      {"bossRecoveryAge",adventure_.bossRecoveryAge},
      {"beamAfterglow",adventure_.beamAfterglowFrames},
      {"run", flow_.runFrames},
      {"deaths", adventure_.deaths},
      {"player",
       {motion_.x, motion_.y, motion_.velocityX, motion_.velocityY,
        motion_.charges, motion_.dashFrames}},
      {"boss",
       {int(adventure_.bossState), adventure_.bossTimer, adventure_.bossX,
        adventure_.bossY, adventure_.bossVisualAngle}},
      {"camera",
       {adventure_.cameraX, adventure_.cameraY, adventure_.cameraDistance}},
      {"ambient", ambientTime_},
      {"hp", adventure_.hp},
      {"score", adventure_.score}};
  j["hazards"] = nlohmann::json::array();
  for (const auto &h : adventure_.hazards)
    j["hazards"].push_back({h.x, h.y, h.life, h.warning});
  j["retiring"] = nlohmann::json::array();
  for(const auto &tail:adventure_.retiringHazards) j["retiring"].push_back({tail.x,tail.y,tail.life});
  j["minions"] = nlohmann::json::array();
  for (const auto &n : adventure_.minions)
    j["minions"].push_back({n.x, n.y, n.life});
  j["particles"] = nlohmann::json::array();
  for (const auto &p : adventure_.particles)
    j["particles"].push_back({p.x, p.y, p.life});
  j["sparks"] = nlohmann::json::array();
  for (const auto &s : effects_.sparks)
    j["sparks"].push_back({s.x, s.y, s.life});
  j["trails"] = nlohmann::json::array();
  for (const auto &t : effects_.trails)
    j["trails"].push_back({t.count, t.fade});
  return j.dump();
}
