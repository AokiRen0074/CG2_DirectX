#pragma once
#include "externals/nlohmann/json.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

struct SpearUiElement {
  std::string id, page = "hud", anchor = "screen", texture, text;
  std::string condition, action, animation, meter;
  std::array<float, 2> position{0, 0}, size{100, 40}, pivot{0, 0},
      spacing{0, 0};
  std::array<float, 4> tint{1, 1, 1, 1}, emptyTint{.1f, .2f, .25f, .7f};
  float fontSize = 24, tracking = 0;
  int repeat = 1;
  bool visible = true;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    SpearUiElement, id, page, anchor, texture, text, condition, action,
    animation, meter, position, size, pivot, spacing, tint, emptyTint, fontSize,
    tracking, repeat, visible)

struct SpearUiContext {
  bool finished = false, boss = false, canReplay = false, newBest = false;
  bool dodge = false, pierce = false, refill = false;
  int hp = 6, charges = 3, phase = 1;
  float bossHp = 1, titleAlpha = 1, titleScale = 1;
  float reveal = 0, bars = 0, flash = 0, impact = 0, blackout = 0;
  float rankReveal = 0, statsReveal = 0;
  float chartReveal = 0, rankScale = 1;
  std::array<int, 5> evaluation{1, 1, 1, 1, 1};
  std::array<float, 2> title{640, 145}, player{380, 520}, feedback{640, 360};
  std::vector<std::pair<std::string, std::string>> values;
  // Only the editor substitutes a preview page; gameplay always uses live
  // conditions.
  std::string preview;
};
struct SpearUiDraw {
  size_t element = 0;
  float x = 0, y = 0, width = 0, height = 0, fontSize = 0, tracking = 0;
  std::array<float, 4> tint{};
  std::string texture, text;
  bool Contains(float px, float py) const {
    return px >= x && py >= y && px <= x + width && py <= y + height;
  }
};

struct SpearUiLayout {
  std::vector<SpearUiElement> elements;
  static bool AssetPath(const std::string &path) {
    return path.starts_with("Resources/UI/") &&
           path.find("..") == std::string::npos &&
           path.find('\\') == std::string::npos && path.ends_with(".png");
  }
  void Validate() const {
    if (elements.empty() || elements.size() > 256)
      throw std::runtime_error("Use 1 to 256 UI elements.");
    std::set<std::string> ids;
    int replayActions = 0;
    for (const auto &e : elements) {
      const auto oneOf = [](const std::string &s,
                            std::initializer_list<const char *> options) {
        return std::any_of(options.begin(), options.end(),
                           [&](auto option) { return s == option; });
      };
      if (e.id.empty() || e.id.size() > 100 || !ids.insert(e.id).second)
        throw std::runtime_error("UI element IDs must be unique and nonempty.");
      if (!oneOf(e.page,
                 {"title", "hud", "boss", "impact", "clear", "transition"}) ||
          !oneOf(e.anchor, {"screen", "title", "player", "feedback"}) ||
          !oneOf(e.condition, {"", "replay", "newBest", "dodge", "pierce",
                               "refill", "lowHealth"}) ||
          !oneOf(e.animation, {"", "reveal", "bars", "flash", "impact",
                               "blackout", "rank", "stats", "chart"}) ||
          !oneOf(e.meter,
                 {"", "hp", "charges", "bossHp", "phase", "evaluation"}) ||
          !oneOf(e.action, {"", "replay"}))
        throw std::runtime_error("Unsupported UI option: " + e.id);
      if (!e.texture.empty() && !AssetPath(e.texture))
        throw std::runtime_error("PNG must be inside Resources/UI: " + e.id);
      if (e.texture.empty() == e.text.empty())
        throw std::runtime_error("Choose either PNG or text: " + e.id);
      if (e.text.size() > 256 || e.repeat < 1 || e.repeat > 16)
        throw std::runtime_error("Text/repeat too large: " + e.id);
      if (e.action == "replay" &&
          (++replayActions > 1 || e.page != "clear" || e.condition != "replay"))
        throw std::runtime_error("Use one replay button on the clear page with "
                                 "the replay condition.");
      const auto check = [&](float v, float lo, float hi) {
        if (!std::isfinite(v) || v < lo || v > hi)
          throw std::runtime_error("UI value out of range: " + e.id);
      };
      for (auto v : e.position)
        check(v, -4096, 4096);
      for (auto v : e.size)
        check(v, 1, 4096);
      for (auto v : e.pivot)
        check(v, 0, 1);
      for (auto v : e.spacing)
        check(v, -1024, 1024);
      for (auto v : e.tint)
        check(v, 0, 1);
      for (auto v : e.emptyTint)
        check(v, 0, 1);
      check(e.fontSize, 6, 200);
      check(e.tracking, -3, 30);
    }
  }
  nlohmann::json Serialize() const {
    Validate();
    return {{"version", 1}, {"canvas", {1280, 720}}, {"elements", elements}};
  }
  static SpearUiLayout Parse(const nlohmann::json &json) {
    if (json.at("version") != 1 ||
        json.at("canvas") != nlohmann::json::array({1280, 720}))
      throw std::runtime_error("Unsupported UI layout version or canvas.");
    SpearUiLayout result{
        json.at("elements").get<std::vector<SpearUiElement>>()};
    result.Validate();
    return result;
  }
  std::vector<SpearUiDraw> Resolve(const SpearUiContext &c) const {
    std::vector<SpearUiDraw> draws;
    for (size_t index = 0; index < elements.size(); ++index) {
      const auto &e = elements[index];
      if (!e.visible)
        continue;
      const bool preview = !c.preview.empty();
      const bool pageVisible =
          preview
              ? (e.page == c.preview ||
                 (c.preview == "title" && e.page == "hud") ||
                 (c.preview == "boss" && e.page == "hud"))
              : e.page == "transition" ||
                    (!c.finished && (e.page == "title" || e.page == "hud" ||
                                     (e.page == "boss" && c.boss))) ||
                    (c.finished && (e.page == "impact" || e.page == "clear"));
      if (!pageVisible)
        continue;
      if ((!preview && ((e.condition == "replay" && !c.canReplay) ||
                        (e.condition == "newBest" && !c.newBest))) ||
          (e.condition == "dodge" && !c.dodge) ||
          (e.condition == "refill" && !c.refill) ||
          (e.condition == "lowHealth" && c.hp > 2) ||
          (e.condition == "pierce" && (!c.pierce || c.dodge)))
        continue;
      float alpha = 1;
      if (!preview) {
        if (e.page == "title")
          alpha *= c.titleAlpha;
        if (e.animation == "reveal")
          alpha *= c.reveal;
        if (e.animation == "bars")
          alpha *= c.bars;
        if (e.animation == "flash")
          alpha *= c.flash;
        if (e.animation == "impact")
          alpha *= c.impact;
        if (e.animation == "blackout")
          alpha *= c.blackout;
        if (e.animation == "rank")
          alpha *= c.rankReveal;
        if (e.animation == "stats")
          alpha *= c.statsReveal;
        if (e.animation == "chart")
          alpha *= c.chartReveal;
      }
      if (alpha <= .001f)
        continue;
      std::array<float, 2> origin{0, 0};
      if (e.anchor == "title")
        origin = c.title;
      if (e.anchor == "player")
        origin = c.player;
      if (e.anchor == "feedback")
        origin = c.feedback;
      const float scale =
          e.anchor == "title"
              ? c.titleScale
              : (!preview && e.animation == "rank" ? c.rankScale : 1);
      std::string text = e.text;
      for (const auto &[name, value] : c.values) {
        const std::string token = "{" + name + "}";
        size_t pos = 0;
        while ((pos = text.find(token, pos)) != std::string::npos) {
          text.replace(pos, token.size(), value);
          pos += value.size();
        }
      }
      for (int n = 0; n < e.repeat; ++n) {
        SpearUiDraw d;
        d.element = index;
        d.texture = e.texture;
        d.text = text;
        d.fontSize = e.fontSize * scale;
        d.tracking = e.tracking * scale;
        d.width = text.empty()
                      ? e.size[0] * scale
                      : (float(text.size()) * (d.fontSize * .6f + d.tracking) -
                         d.tracking + d.fontSize * .4f);
        d.height = text.empty() ? e.size[1] * scale : d.fontSize * 1.5f;
        d.x = origin[0] + (e.position[0] + float(n) * e.spacing[0]) * scale -
              d.width * e.pivot[0];
        d.y = origin[1] + (e.position[1] + float(n) * e.spacing[1]) * scale -
              d.height * e.pivot[1];
        if (!preview && e.animation == "reveal")
          d.y += (1 - c.reveal) * 24;
        if (e.meter == "bossHp")
          d.width *= (std::clamp)(c.bossHp, 0.0f, 1.0f);
        const int amount = e.meter == "hp"        ? c.hp
                           : e.meter == "charges" ? c.charges
                                                  : c.phase;
        d.tint = !e.meter.empty() && e.meter != "bossHp" &&
                         e.meter != "evaluation" && n >= amount
                     ? e.emptyTint
                     : e.tint;
        d.tint[3] *= alpha;
        draws.push_back(d);
      }
    }
    return draws;
  }
};
