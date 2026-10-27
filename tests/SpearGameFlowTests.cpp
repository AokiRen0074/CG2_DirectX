#include "SpearGameFlow.h"
#include <cassert>
#include <iostream>
#include <string_view>

int main() {
  SpearPauseState pause;
  assert(!pause.active && pause.Confirm()==SpearPauseState::Action::None);
  pause.Open();assert(pause.active && pause.selection==0);
  pause.Move(-1);assert(pause.selection==1 && pause.Confirm()==SpearPauseState::Action::Retry);
  pause.Move(1);assert(pause.selection==0 && pause.Confirm()==SpearPauseState::Action::Resume);
  pause.Close();pause.Move(1);assert(!pause.active && pause.selection==0);
  pause.Open();assert(pause.selection==0);

  SpearRestartTransition restart;
  assert(!restart.Active() && restart.Alpha() == 0 && !restart.Tick());
  restart.Begin(false);
  assert(restart.Active() && restart.Alpha() == 0 && !restart.fullRun);
  restart.Begin(true);
  assert(!restart.fullRun); // Repeated input cannot interrupt a transition.
  int resets = 0;
  for (int frame = 1; frame <= 60; ++frame) {
    const bool reset = restart.Tick();
    if (reset) {
      ++resets;
      assert(frame == 24 && restart.Alpha() == 1);
    }
    if (frame == 12 || frame == 45)
      assert(restart.Alpha() > 0 && restart.Alpha() < 1);
    if (frame >= 24 && frame <= 30)
      assert(restart.Alpha() == 1);
  }
  assert(resets == 1 && !restart.Active() && restart.Alpha() == 0);
  restart.Begin(true);
  assert(restart.Active() && restart.fullRun);
  SpearGameFlow flow;
  assert(!flow.Finished() && !flow.CanReplay());
  assert(SpearGameFlow::TitleAlpha(0, 0) == 1);
  assert(SpearGameFlow::TitleAlpha(12, 0) > 0 &&
         SpearGameFlow::TitleAlpha(12, 0) < 1);
  assert(SpearGameFlow::TitleAlpha(22, 0) == 0);
  for (int i = 0; i < 120; ++i)
    flow.Tick(false);
  flow.Tick(true);
  assert(flow.Finished() && flow.runFrames == 121 && flow.clearFrames == 0 &&
         flow.newBest);
  for (int i = 0; i < 229; ++i)
    flow.Tick(true);
  assert(!flow.CanReplay() && flow.runFrames == 121);
  flow.Tick(true);
  assert(flow.CanReplay() && flow.bestFrames == 121);
  for (int i = 0; i < 500; ++i)
    flow.Tick(true);
  assert(flow.runFrames == 121 && flow.bestFrames == 121);
  flow.Reset();
  assert(!flow.Finished() && !flow.CanReplay() && flow.runFrames == 0 &&
         flow.bestFrames == 121);
  for (int i = 0; i < 240; ++i)
    flow.Tick(false);
  flow.Tick(true);
  assert(!flow.newBest && flow.bestFrames == 121);
  flow.Reset();
  for (int i = 0; i < 60; ++i)
    flow.Tick(false);
  flow.Tick(true);
  assert(flow.newBest && flow.bestFrames == 61);
  const char *ranks[] = {"D", "C", "B", "A", "S"};
  for (int level = 1; level <= 5; ++level) {
    SpearRunEvaluation evaluation;
    evaluation.levels.fill(level);
    assert(std::string_view(evaluation.Rank()) == ranks[level - 1]);
  }
  auto poor = SpearRunEvaluation::Calculate(400 * 60, 0, 0, 0, 0);
  assert(std::string_view(poor.Rank()) == "D");
  auto good = SpearRunEvaluation::Calculate(80 * 60, 15, 8, 8, 5);
  for (int level : good.levels)
    assert(level == 5);
  assert(std::string_view(good.Rank()) == "S");
  auto practice = SpearRunEvaluation::Calculate(60, 1, 0, 0, 0);
  assert(std::string_view(practice.Rank()) == "C");
  std::cout << "PASS: run clock, world-title fade, clear timing, replay gate, "
               "rank and "
               "session-best retention.\n";
}
