#pragma once
#include <algorithm>
#include <array>

struct SpearRunEvaluation {
  // Clockwise from the top: attack, dodge, speed, pierce, chain. Each is 1..5.
  std::array<int, 5> levels{1, 1, 1, 1, 1};
  static SpearRunEvaluation Calculate(int frames, int hits, int dodges,
                                      int chain, int pierce) {
    const auto tier = [](int value, std::array<int, 4> limits) {
      int result = 1;
      for (int limit : limits)
        if (value >= limit)
          ++result;
      return result;
    };
    const int seconds = (std::max)(0, frames) / 60;
    const int speed = seconds <= 90    ? 5
                      : seconds <= 150 ? 4
                      : seconds <= 210 ? 3
                      : seconds <= 300 ? 2
                                       : 1;
    return {{tier(hits, {3, 6, 10, 15}), tier(dodges, {1, 3, 5, 8}), speed,
             tier(pierce, {2, 3, 4, 5}), tier(chain, {2, 4, 6, 8})}};
  }
  const char *Rank() const {
    int total = 0;
    for (int value : levels)
      total += (std::clamp)(value, 1, 5);
    return total >= 23   ? "S"
           : total >= 18 ? "A"
           : total >= 13 ? "B"
           : total >= 8  ? "C"
                         : "D";
  }
};

// Run progression stays inside GameScene. Editors pause this clock with
// gameplay.
struct SpearGameFlow {
  int runFrames = 0;
  int clearFrames = -1;
  int bestFrames = 0;
  bool newBest = false;

  void Reset() {
    runFrames = 0;
    clearFrames = -1;
    newBest = false;
  }
  void Tick(bool won) {
    if (clearFrames >= 0) {
      ++clearFrames;
      return;
    }
    ++runFrames;
    if (won) {
      clearFrames = 0;
      newBest = bestFrames == 0 || runFrames < bestFrames;
      if (newBest)
        bestFrames = runFrames;
    }
  }
  bool Finished() const { return clearFrames >= 0; }
  bool CanReplay() const { return clearFrames >= 230; }
  static float Ease(float from, float to, float value) {
    const float t = (std::clamp)((value - from) / (to - from), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
  }
  static float TitleAlpha(float cameraX, float startX) {
    return 1.0f - Ease(startX + 6, startX + 20, cameraX);
  }
};

// Change the stage only while the screen is fully black.
struct SpearRestartTransition {
  int frame = -1;
  bool fullRun = true;

  bool Active() const { return frame >= 0; }
  void Begin(bool restartWholeRun) {
    if (Active())
      return;
    fullRun = restartWholeRun;
    frame = 0;
  }
  bool Tick() {
    if (!Active())
      return false;
    ++frame;
    const bool applyRestart = frame == 24;
    if (frame >= 60)
      frame = -1;
    return applyRestart;
  }
  float Alpha() const {
    if (!Active())
      return 0;
    if (frame <= 24)
      return SpearGameFlow::Ease(0, 24, float(frame));
    return 1 - SpearGameFlow::Ease(30, 60, float(frame));
  }
};

// UI state only. While active, the owner skips every simulation update.
struct SpearPauseState {
  enum class Action { None, Resume, Retry };
  bool active=false;
  int selection=0;
  void Open() {active=true;selection=0;}
  void Close() {active=false;}
  void Move(int direction) {
    if(active && direction!=0) selection=(selection+(direction>0?1:-1)+2)%2;
  }
  Action Confirm() {
    if(!active) return Action::None;
    const auto result=selection==0?Action::Resume:Action::Retry;
    return result;
  }
};
