#include "SpearUiLayout.h"
#include <cassert>
#include <fstream>
#include <iostream>

int main(int argc, char **argv) {
  assert(argc == 2);
  std::ifstream file(argv[1]);
  nlohmann::json json;
  file >> json;
  auto layout = SpearUiLayout::Parse(json);
  assert(SpearUiLayout::Parse(layout.Serialize()).Serialize() ==
         layout.Serialize());
  auto find = [&](const std::string &id) -> SpearUiElement & {
    for (auto &e : layout.elements)
      if (e.id == id)
        return e;
    throw std::runtime_error("Missing element " + id);
  };
  auto contains = [&](const auto &draws, const char *id) {
    return std::any_of(draws.begin(), draws.end(), [&](const auto &d) {
      return layout.elements[d.element].id == id;
    });
  };
  SpearUiContext c;
  c.values = {{"time", "01:23.45"},
              {"score6", "012345"},
              {"score", "12345"},
              {"rank", "S"}};
  auto draws = layout.Resolve(c);
  assert(contains(draws, "title_logo") && contains(draws, "score_value"));
  assert(!contains(draws, "new_best") && !contains(draws, "replay_button") &&
         !contains(draws, "refill_feedback"));
  assert(!contains(draws, "health_critical"));
  for (const auto &d : draws)
    if (layout.elements[d.element].id == "score_value")
      assert(d.text == "012345");
  c.titleAlpha = 0;
  assert(!contains(layout.Resolve(c), "title_logo"));
  c.titleAlpha = 1;
  c.title = {800, 200};
  const auto title = find("title_logo");
  for (const auto &d : layout.Resolve(c))
    if (layout.elements[d.element].id == "title_logo")
      assert(std::abs(d.x - (800 - title.size[0] * .5f)) < .001f);
  c.hp = 2;
  assert(contains(layout.Resolve(c), "health_critical"));
  int full = 0, empty = 0;
  for (const auto &d : layout.Resolve(c))
    if (layout.elements[d.element].id == "health_icons") {
      if (d.tint == find("health_icons").tint)
        ++full;
      else
        ++empty;
    }
  assert(full == 2 && empty == 4);
  c.boss = true;
  c.bossHp = .5f;
  for (const auto &d : layout.Resolve(c))
    if (layout.elements[d.element].id == "boss_health_fill")
      assert(d.width == find("boss_health_fill").size[0] * .5f);
  c.finished = true;
  c.reveal = 1;
  assert(contains(layout.Resolve(c), "result_heading") &&
         !contains(layout.Resolve(c), "rank_value") &&
         !contains(layout.Resolve(c), "result_score_value"));
  c.statsReveal = c.chartReveal = 1;
  assert(contains(layout.Resolve(c), "evaluation_area") &&
         !contains(layout.Resolve(c), "rank_value"));
  c.rankReveal = 1;
  assert(contains(layout.Resolve(c), "result_heading") &&
         !contains(layout.Resolve(c), "title_logo"));
  assert(!contains(layout.Resolve(c), "replay_button"));
  c.canReplay = true;
  c.newBest = true;
  auto &button = find("replay_button");
  button.position = {250, 420};
  button.size = {300, 65};
  bool clicked = false;
  for (const auto &d : layout.Resolve(c))
    if (layout.elements[d.element].action == "replay") {
      assert(d.Contains(251, 421) && !d.Contains(600, 421));
      clicked = true;
    }
  assert(clicked && !contains(layout.Resolve(c), "new_best"));
  button.visible = false;
  assert(!contains(layout.Resolve(c), "replay_button"));
  button.visible = true;
  c.blackout = 1;
  draws = layout.Resolve(c);
  assert(layout.elements[draws.back().element].id == "restart_blackout" &&
         draws.back().tint[3] == 1);
  c.preview = "clear";
  c.finished = false;
  c.reveal = 0;
  c.canReplay = false;
  assert(contains(layout.Resolve(c), "replay_button") &&
         !contains(layout.Resolve(c), "time_label"));
  const auto mustReject = [&](nlohmann::json bad) {
    bool rejected = false;
    try {
      (void)SpearUiLayout::Parse(bad);
    } catch (const std::exception &) {
      rejected = true;
    }
    assert(rejected);
  };
  auto bad = json;
  bad["elements"][1]["id"] = bad["elements"][0]["id"];
  mustReject(bad);
  bad = json;
  bad["elements"][0]["texture"] = "Resources/UI/../../outside.png";
  mustReject(bad);
  bad = json;
  bad["elements"][0]["size"] = {-1, 30};
  mustReject(bad);
  bad = json;
  bad["elements"][0]["repeat"] = 100000;
  mustReject(bad);
  bad = json;
  bad["version"] = 9;
  mustReject(bad);
  std::cout << "PASS: UI resources, round-trip, bindings, anchors, meters, "
               "result conditions, moved-button hit area, blackout order, "
               "previews and validation.\n";
}
