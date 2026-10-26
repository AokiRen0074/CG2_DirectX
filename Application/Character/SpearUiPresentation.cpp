#include "Base/WindowApp.h"
#include "Input/Input.h"
#include "SpearPrototype.h"
#include <Windows.h>
#include <filesystem>
#include <format>
#include <fstream>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

namespace {
constexpr const char *layoutResource = "Resources/UI/PrismUI/layout.json";
std::filesystem::path SourceLayout() {
  auto folder = std::filesystem::current_path();
  for (;;) {
    if (std::filesystem::exists(folder / "CG2_DirectX.sln"))
      return folder / layoutResource;
    auto parent = folder.parent_path();
    if (parent == folder)
      break;
    folder = parent;
  }
  return std::filesystem::absolute(layoutResource);
}
std::string TimeText(int frames) {
  return std::format("{:02}:{:02}.{:02}", frames / 3600, (frames / 60) % 60,
                     (frames % 60) * 100 / 60);
}
void WriteLayout(const std::filesystem::path &path,
                 const SpearUiLayout &layout) {
  const auto temporary = std::filesystem::path(path.wstring() + L".tmp");
  const std::string bytes = layout.Serialize().dump(2) + '\n';
  std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
  file.write(bytes.data(), std::streamsize(bytes.size()));
  file.close();
  if (!file)
    throw std::runtime_error("Could not write UI layout.");
  if (std::filesystem::exists(path))
    std::filesystem::copy_file(
        path, path.wstring() + L".bak",
        std::filesystem::copy_options::overwrite_existing);
  if (!MoveFileExW(temporary.c_str(), path.c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    throw std::runtime_error(
        "Could not replace UI layout. Previous file kept.");
}
} // namespace

void SpearPrototype::PrepareUiAssets(const SpearUiLayout &layout) {
  const auto source = SourceLayout();
  const auto root =
      source.parent_path().parent_path().parent_path().parent_path();
  for (const auto &e : layout.elements) {
    if (e.texture.empty())
      continue;
    const auto actual = std::filesystem::exists(root / e.texture)
                            ? root / e.texture
                            : std::filesystem::path(e.texture);
    if (!std::filesystem::exists(actual))
      throw std::runtime_error("PNG not found: " + e.texture);
    screen_.PrepareImage(e.texture, actual.string());
  }
}
void SpearPrototype::LoadUiLayout(bool fromSource) {
  std::ifstream file(fromSource ? SourceLayout()
                                : std::filesystem::path(layoutResource));
  if (!file)
    throw std::runtime_error(
        "UI layout missing: Resources/UI/PrismUI/layout.json");
  nlohmann::json json;
  file >> json;
  auto loaded = SpearUiLayout::Parse(json);
  PrepareUiAssets(loaded);
  uiLayout_ = std::move(loaded);
}
void SpearPrototype::OpenUiEditor(const char *page) {
#ifdef USE_IMGUI
  uiEditorOpen_ = true;
  editorOpen_ = stageEditorOpen_ = false;
  uiPreviewPage_ = page;
  pendingJump_ = jumpHeld_ = false;
  accumulator_ = 0;
  uiStatus_ = "Drag to move. Corner to resize. Ctrl+S saves to Resources.";
  uiAssets_.clear();
  const auto source = SourceLayout();
  const auto root =
      source.parent_path().parent_path().parent_path().parent_path();
  for (const auto &entry :
       std::filesystem::recursive_directory_iterator(root / "Resources/UI"))
    if (entry.is_regular_file() && entry.path().extension() == ".png")
      uiAssets_.push_back(
          std::filesystem::relative(entry.path(), root).generic_string());
  std::sort(uiAssets_.begin(), uiAssets_.end());
#else
  (void)page;
#endif
}
void SpearPrototype::RememberUiEdit() {
  if (uiUndo_.size() >= 64)
    uiUndo_.erase(uiUndo_.begin());
  uiUndo_.push_back(uiLayout_);
  uiRedo_.clear();
  uiDirty_ = true;
}
SpearUiContext SpearPrototype::UiContext(bool preview) const {
  SpearUiContext c;
  c.finished = flow_.Finished();
  c.canReplay = flow_.CanReplay();
  c.newBest = flow_.newBest;
  c.hp = adventure_.hp;
  c.charges = motion_.charges;
  c.boss = adventure_.bossState != SpearAdventure::BossState::Dormant;
  c.bossHp = float(adventure_.bossHp) / SpearAdventure::bossMaxHp;
  c.phase = adventure_.secondPhase ? 2 : 1;
  c.dodge = adventure_.dodgeFlash > 0;
  c.pierce = adventure_.pierceFlash > 0 && adventure_.pierceCount >= 2;
  c.refill = adventure_.chargeRefillFlash > 0;
  const float t = float(flow_.clearFrames);
  c.reveal = SpearGameFlow::Ease(60, 90, t);
  c.chartReveal = SpearGameFlow::Ease(90, 145, t);
  c.rankReveal = SpearGameFlow::Ease(180, 184, t);
  c.rankScale = 1 + .65f * (1 - SpearGameFlow::Ease(180, 188, t)) -
                .08f * (SpearGameFlow::Ease(188, 194, t) -
                        SpearGameFlow::Ease(194, 200, t));
  c.statsReveal = SpearGameFlow::Ease(90, 120, t);
  const auto evaluation = SpearRunEvaluation::Calculate(
      flow_.runFrames, adventure_.attackHits, adventure_.dodgeCount,
      adventure_.maximumChain, adventure_.bestPierce);
  c.evaluation = evaluation.levels;
  c.bars = SpearGameFlow::Ease(0, 45, t);
  c.flash = t >= 0 && t < 10 ? (1 - t / 10) * (1 - t / 10) : 0;
  c.impact =
      SpearGameFlow::Ease(35, 60, t) * (1 - SpearGameFlow::Ease(95, 120, t));
  c.blackout = restartTransition_.Alpha();
  const auto &start = adventure_.stage.spawns[0];
  c.titleAlpha = SpearGameFlow::TitleAlpha(adventure_.cameraX, start.x + 5);
  const Vector3 title = Transform(
      {start.x + 5, start.y + 9.7f, 2},
      Multiply(dashGaugeCamera_.matView, dashGaugeCamera_.matProjection));
  c.title = {(title.x * .5f + .5f) * 1280, (.5f - title.y * .5f) * 720};
  c.titleScale = 34.0f / (adventure_.cameraDistance + 2);
  c.player = dashGaugeScreenPoint_;
  c.feedback = feedbackScreenPoint_;
  c.values = {{"time", TimeText(flow_.runFrames)},
              {"score", std::to_string(adventure_.score)},
              {"bestTime", TimeText(flow_.bestFrames)},
              {"score6", std::format("{:06}", adventure_.score)},
              {"chain", std::to_string(adventure_.maximumChain)},
              {"retries", std::to_string(adventure_.deaths)},
              {"rank", evaluation.Rank()},
              {"attackRating", std::to_string(c.evaluation[0])},
              {"dodgeRating", std::to_string(c.evaluation[1])},
              {"speedRating", std::to_string(c.evaluation[2])},
              {"pierceRating", std::to_string(c.evaluation[3])},
              {"chainRating", std::to_string(c.evaluation[4])},
              {"pierce", std::to_string(adventure_.pierceCount)}};
  if (preview) {
    c.preview = uiPreviewPage_;
    c.title = {640, 145};
    c.titleScale = 1;
    c.player = {380, 520};
    c.feedback = {640, 400};
    c.hp = 4;
    c.charges = 2;
    c.bossHp = .65f;
    c.phase = 2;
    c.evaluation = {4, 3, 5, 2, 4};
    const std::string selectedCondition =
        uiSelection_ >= 0 && uiSelection_ < int(uiLayout_.elements.size())
            ? uiLayout_.elements[size_t(uiSelection_)].condition
            : "";
    c.dodge = selectedCondition == "dodge";
    c.pierce = selectedCondition == "pierce";
    c.refill = selectedCondition == "refill";
    if (selectedCondition == "lowHealth")
      c.hp = 2;
    c.values = {
        {"time", "01:23.45"}, {"score", "12800"},       {"score6", "012800"},
        {"chain", "7"},       {"retries", "1"},         {"rank", "A"},
        {"pierce", "3"},      {"bestTime", "01:20.18"}, {"attackRating", "4"},
        {"dodgeRating", "3"}, {"speedRating", "5"},     {"pierceRating", "2"},
        {"chainRating", "4"}};
  }
  return c;
}
bool SpearPrototype::UiReplayHovered(float x, float y) const {
  for (const auto &d : uiLayout_.Resolve(UiContext(false)))
    if (uiLayout_.elements[d.element].action == "replay" && d.Contains(x, y))
      return true;
  return false;
}
void SpearPrototype::DrawPresentation() {
  if (editorOpen_ || stageEditorOpen_)
    return;
  screen_.Begin();
  const bool editing = uiEditorOpen_;
  const float scale = editing ? 900.0f / 1280 : 1;
  const float offsetX = editing ? 10.0f : 0, offsetY = editing ? 104.0f : 0;
  if (editing)
    screen_.Rect(offsetX, offsetY, 900, 506.25f, {.018f, .027f, .045f, 1});
  const auto context = UiContext(editing);
  for (const auto &d : uiLayout_.Resolve(context)) {
    auto color = d.tint;
    if (!editing && replayHovered_ &&
        uiLayout_.elements[d.element].action == "replay")
      color[3] = 1;
    Vector4 tint{color[0], color[1], color[2], color[3]};
    if (uiLayout_.elements[d.element].meter == "evaluation")
      screen_.Radar(offsetX + d.x * scale, offsetY + d.y * scale,
                    d.width * scale, d.height * scale, context.evaluation,
                    editing ? 1 : context.chartReveal, tint);
    else if (!d.texture.empty())
      screen_.Image(d.texture, offsetX + d.x * scale, offsetY + d.y * scale,
                    d.width * scale, d.height * scale, tint);
    else
      screen_.Text(d.text, offsetX + d.x * scale, offsetY + d.y * scale,
                   d.fontSize * scale, tint, d.tracking * scale);
  }
  screen_.Draw(editing ? 10 : 0, editing ? 104 : 0, editing ? 900 : 1280,
               editing ? 507 : 720);
}

void SpearPrototype::DrawUiEditor() {
#ifdef USE_IMGUI
  auto &io = ImGui::GetIO();
  const auto flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoCollapse;
  const auto save = [&] {
    try {
      PrepareUiAssets(uiLayout_);
      const auto source = uiSaveOverride_.empty()
                              ? SourceLayout()
                              : std::filesystem::absolute(uiSaveOverride_);
      std::filesystem::create_directories(source.parent_path());
      WriteLayout(source, uiLayout_);
      if (uiSaveOverride_.empty() &&
          std::filesystem::weakly_canonical(source) !=
              std::filesystem::weakly_canonical(layoutResource))
        WriteLayout(layoutResource, uiLayout_);
      uiDirty_ = false;
      uiStatus_ = "Saved: " + source.generic_string();
    } catch (const std::exception &e) {
      uiStatus_ = std::string("Save failed: ") + e.what();
    }
  };
  const auto undo = [&](bool redo) {
    auto &from = redo ? uiRedo_ : uiUndo_;
    auto &to = redo ? uiUndo_ : uiRedo_;
    if (!from.empty()) {
      to.push_back(uiLayout_);
      uiLayout_ = from.back();
      from.pop_back();
      uiDirty_ = true;
      uiSelection_ = -1;
    }
  };
  if (!io.WantTextInput && io.KeyCtrl) {
    if (Input::GetInstance()->TriggerKey(DIK_S))
      save();
    if (Input::GetInstance()->TriggerKey(DIK_Z))
      undo(false);
    if (Input::GetInstance()->TriggerKey(DIK_Y))
      undo(true);
  }
  ImGui::SetNextWindowPos({10, 10}, ImGuiCond_Always);
  ImGui::SetNextWindowSize({900, 88}, ImGuiCond_Always);
  ImGui::Begin(uiDirty_ ? "UI Layout Editor *###UiToolbar"
                        : "UI Layout Editor###UiToolbar",
               nullptr, flags);
  if (ImGui::Button("Save (Ctrl+S)"))
    save();
  ImGui::SameLine();
  if (ImGui::Button("Reload from resource"))
    ImGui::OpenPopup("Reload layout?");
  if (ImGui::BeginPopupModal("Reload layout?", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextUnformatted("Discard unsaved changes and reload layout.json?");
    if (ImGui::Button("Reload")) {
      try {
        RememberUiEdit();
        LoadUiLayout(true);
        uiSelection_ = -1;
        uiDirty_ = false;
        uiStatus_ = "Reloaded from source resource.";
      } catch (const std::exception &e) {
        uiStatus_ = e.what();
      }
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
      ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Undo"))
    undo(false);
  ImGui::SameLine();
  if (ImGui::Button("Redo"))
    undo(true);
  ImGui::SameLine();
  if (ImGui::Button("Return to game (F4)"))
    uiEditorOpen_ = false;
  const char *pages[] = {"title",  "hud",   "boss",
                         "impact", "clear", "transition"};
  ImGui::TextUnformatted("Preview:");
  for (auto page : pages) {
    ImGui::SameLine(0, 5);
    if (ImGui::RadioButton(page, uiPreviewPage_ == page)) {
      uiPreviewPage_ = page;
      uiSelection_ = -1;
    }
  }
  ImGui::End();

  ImGui::SetNextWindowPos({920, 10}, ImGuiCond_Always);
  ImGui::SetNextWindowSize({350, 700}, ImGuiCond_Always);
  ImGui::Begin("UI Properties", nullptr, flags);
  ImGui::TextUnformatted("Elements (back to front)");
  ImGui::BeginChild("UiElements", {0, 156}, true);
  const auto context = UiContext(true);
  const auto draws = uiLayout_.Resolve(context);
  for (size_t i = 0; i < uiLayout_.elements.size(); ++i) {
    auto &e = uiLayout_.elements[i];
    if (e.page != uiPreviewPage_ &&
        !(uiPreviewPage_ == "title" && e.page == "hud") &&
        !(uiPreviewPage_ == "boss" && e.page == "hud"))
      continue;
    ImGui::PushID(int(i));
    if (ImGui::Selectable(e.id.c_str(), uiSelection_ == int(i)))
      uiSelection_ = int(i);
    ImGui::PopID();
  }
  ImGui::EndChild();
  if (ImGui::Button("Add image") && uiLayout_.elements.size() < 256) {
    RememberUiEdit();
    SpearUiElement e;
    e.id = "image_" + std::to_string(++uiNewId_);
    while (std::any_of(uiLayout_.elements.begin(), uiLayout_.elements.end(),
                       [&](const auto &v) { return v.id == e.id; }))
      e.id = "image_" + std::to_string(++uiNewId_);
    e.page = uiPreviewPage_;
    e.texture = "Resources/UI/PrismUI/panel.png";
    e.position = {400, 300};
    uiLayout_.elements.push_back(e);
    uiSelection_ = int(uiLayout_.elements.size()) - 1;
  }
  ImGui::SameLine();
  if (ImGui::Button("Add text") && uiLayout_.elements.size() < 256) {
    RememberUiEdit();
    SpearUiElement e;
    e.id = "text_" + std::to_string(++uiNewId_);
    while (std::any_of(uiLayout_.elements.begin(), uiLayout_.elements.end(),
                       [&](const auto &v) { return v.id == e.id; }))
      e.id = "text_" + std::to_string(++uiNewId_);
    e.page = uiPreviewPage_;
    e.text = "TEXT";
    e.position = {400, 300};
    uiLayout_.elements.push_back(e);
    uiSelection_ = int(uiLayout_.elements.size()) - 1;
  }
  if (uiSelection_ >= 0 && uiSelection_ < int(uiLayout_.elements.size())) {
    auto &e = uiLayout_.elements[size_t(uiSelection_)];
    ImGui::Separator();
    ImGui::TextUnformatted(e.id.c_str());
    // A whole widget gesture occupies one undo slot, even for continuous drags.
    const SpearUiLayout before = uiLayout_;
    bool changed = false, activated = false;
    const auto track = [&](bool value) {
      changed |= value;
      activated |= ImGui::IsItemActivated();
    };
    track(ImGui::Checkbox("Visible", &e.visible));
    ImGui::SetNextItemWidth(210);
    if (ImGui::BeginCombo("Page", e.page.c_str())) {
      for (auto page : pages)
        if (ImGui::Selectable(page, e.page == page)) {
          e.page = page;
          changed = true;
          activated = true;
        }
      ImGui::EndCombo();
    }
    const auto combo = [&](const char *label, std::string &value,
                           std::initializer_list<const char *> options) {
      ImGui::SetNextItemWidth(210);
      if (ImGui::BeginCombo(label, value.empty() ? "None" : value.c_str())) {
        for (auto option : options)
          if (ImGui::Selectable(*option ? option : "None", value == option)) {
            value = option;
            changed = true;
            activated = true;
          }
        ImGui::EndCombo();
      }
    };
    combo("Anchor", e.anchor, {"screen", "title", "player", "feedback"});
    track(ImGui::DragFloat2("Position", e.position.data(), 1, -4096, 4096,
                            "%.0f"));
    track(ImGui::DragFloat2("Size", e.size.data(), 1, 1, 4096, "%.0f"));
    track(ImGui::SliderFloat2("Pivot", e.pivot.data(), 0, 1, "%.2f"));
    track(ImGui::ColorEdit4("Tint / alpha", e.tint.data()));
    if (!e.texture.empty()) {
      ImGui::SetNextItemWidth(210);
      if (ImGui::BeginCombo(
              "PNG asset",
              std::filesystem::path(e.texture).filename().string().c_str())) {
        for (const auto &path : uiAssets_)
          if (ImGui::Selectable(path.c_str(), path == e.texture)) {
            try {
              auto proposed = uiLayout_;
              proposed.elements[size_t(uiSelection_)].texture = path;
              PrepareUiAssets(proposed);
              e.texture = path;
              changed = true;
              activated = true;
            } catch (const std::exception &error) {
              uiStatus_ = error.what();
            }
          }
        ImGui::EndCombo();
      }
    } else {
      char buffer[257]{};
      std::copy(e.text.begin(), e.text.end(), buffer);
      track(ImGui::InputText("Text / token", buffer, sizeof(buffer)));
      e.text = buffer;
      track(ImGui::SliderFloat("Font size", &e.fontSize, 6, 200, "%.0f"));
      track(ImGui::SliderFloat("Tracking", &e.tracking, -3, 30, "%.1f"));
    }
    combo("Condition", e.condition,
          {"", "replay", "newBest", "dodge", "pierce", "refill", "lowHealth"});
    combo("Animation", e.animation,
          {"", "reveal", "bars", "flash", "impact", "blackout", "rank", "stats",
           "chart"});
    combo("Meter", e.meter,
          {"", "hp", "charges", "bossHp", "phase", "evaluation"});
    track(ImGui::SliderInt("Repeat", &e.repeat, 1, 16));
    track(ImGui::DragFloat2("Repeat gap", e.spacing.data(), 1, -1024, 1024,
                            "%.0f"));
    if (!e.meter.empty())
      track(ImGui::ColorEdit4("Empty tint", e.emptyTint.data()));
    if (activated) {
      if (uiUndo_.size() >= 64)
        uiUndo_.erase(uiUndo_.begin());
      uiUndo_.push_back(before);
      uiRedo_.clear();
    }
    if (changed)
      uiDirty_ = true;
    if (e.action == "replay")
      ImGui::TextWrapped(
          "Replay button: the clickable area follows Position and Size.");
    if (ImGui::Button("Duplicate") && uiLayout_.elements.size() < 256) {
      auto copy = e;
      copy.action.clear();
      copy.position[0] += 16;
      copy.position[1] += 16;
      do {
        copy.id = e.id + "_copy" + std::to_string(++uiNewId_);
      } while (std::any_of(uiLayout_.elements.begin(), uiLayout_.elements.end(),
                           [&](const auto &v) { return v.id == copy.id; }));
      RememberUiEdit();
      uiLayout_.elements.push_back(copy);
      uiSelection_ = int(uiLayout_.elements.size()) - 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete") && uiLayout_.elements.size() > 1) {
      RememberUiEdit();
      uiLayout_.elements.erase(uiLayout_.elements.begin() + uiSelection_);
      uiSelection_ = -1;
    }
    if (uiSelection_ >= 0) {
      if (ImGui::Button("Layer back") && uiSelection_ > 0) {
        RememberUiEdit();
        std::swap(uiLayout_.elements[size_t(uiSelection_)],
                  uiLayout_.elements[size_t(uiSelection_ - 1)]);
        --uiSelection_;
      }
      ImGui::SameLine();
      if (ImGui::Button("Layer front") &&
          uiSelection_ + 1 < int(uiLayout_.elements.size())) {
        RememberUiEdit();
        std::swap(uiLayout_.elements[size_t(uiSelection_)],
                  uiLayout_.elements[size_t(uiSelection_ + 1)]);
        ++uiSelection_;
      }
    }
  }
  ImGui::End();

  ImGui::SetNextWindowPos({10, 618}, ImGuiCond_Always);
  ImGui::SetNextWindowSize({900, 92}, ImGuiCond_Always);
  ImGui::Begin("UI Help / Save status", nullptr, flags);
  ImGui::TextWrapped("Canvas 1280 x 720. Click to select; drag to move; drag "
                     "bottom-right handle to resize. Shift: 8px grid. Arrow "
                     "keys: nudge (Shift: 8px). Ctrl+Z/Y: undo/redo.");
  ImGui::TextWrapped("%s", uiStatus_.c_str());
  ImGui::End();

  const float scale = 900.0f / 1280;
  const ImVec2 mouse = io.MousePos;
  const bool inCanvas =
      mouse.x >= 10 && mouse.x < 910 && mouse.y >= 104 && mouse.y < 610.25f;
  if (ImGui::IsMouseClicked(0))
    uiCanvasFocused_ = inCanvas && !io.WantCaptureMouse;
  if (!io.WantCaptureMouse && inCanvas && ImGui::IsMouseClicked(0)) {
    uiDragging_ = false;
    uiResizing_ = false;
    if (uiSelection_ >= 0)
      for (const auto &d : draws)
        if (int(d.element) == uiSelection_) {
          const float right = 10 + (d.x + d.width) * scale,
                      bottom = 104 + (d.y + d.height) * scale;
          if (std::abs(mouse.x - right) < 9 && std::abs(mouse.y - bottom) < 9)
            uiResizing_ = true;
        }
    if (!uiResizing_) {
      uiSelection_ = -1;
      for (auto it = draws.rbegin(); it != draws.rend(); ++it)
        if (it->Contains((mouse.x - 10) / scale, (mouse.y - 104) / scale)) {
          uiSelection_ = int(it->element);
          break;
        }
    }
    if (uiSelection_ >= 0) {
      uiDragRecorded_ = false;
      uiDragging_ = true;
      uiDragStart_ = {mouse.x, mouse.y};
      uiDragOriginal_ = uiLayout_.elements[size_t(uiSelection_)];
    }
  }
  if (uiDragging_ && ImGui::IsMouseDown(0) && uiSelection_ >= 0 &&
      (uiDragRecorded_ || std::abs(mouse.x - uiDragStart_[0]) > .1f ||
       std::abs(mouse.y - uiDragStart_[1]) > .1f)) {
    if (!uiDragRecorded_) {
      RememberUiEdit();
      uiDragRecorded_ = true;
    }
    auto &e = uiLayout_.elements[size_t(uiSelection_)];
    const float dx = (mouse.x - uiDragStart_[0]) / scale,
                dy = (mouse.y - uiDragStart_[1]) / scale;
    const auto snap = [&](float value) {
      return io.KeyShift ? std::round(value / 8) * 8 : value;
    };
    if (uiResizing_) {
      if (e.texture.empty())
        e.fontSize = (std::clamp)(snap(uiDragOriginal_.fontSize + dy / 1.5f),
                                  6.0f, 200.0f);
      else {
        e.size[0] =
            (std::clamp)(snap(uiDragOriginal_.size[0] + dx), 1.0f, 4096.0f);
        e.size[1] =
            (std::clamp)(snap(uiDragOriginal_.size[1] + dy), 1.0f, 4096.0f);
        for (size_t axis = 0; axis < 2; ++axis)
          e.position[axis] = (std::clamp)(
              uiDragOriginal_.position[axis] +
                  (e.size[axis] - uiDragOriginal_.size[axis]) * e.pivot[axis],
              -4096.0f, 4096.0f);
      }
    } else {
      e.position[0] = (std::clamp)(snap(uiDragOriginal_.position[0] + dx),
                                   -4096.0f, 4096.0f);
      e.position[1] = (std::clamp)(snap(uiDragOriginal_.position[1] + dy),
                                   -4096.0f, 4096.0f);
    }
  }
  if (!ImGui::IsMouseDown(0))
    uiDragging_ = false;
  if (uiSelection_ >= 0 && uiCanvasFocused_ && !io.WantTextInput &&
      !ImGui::IsAnyItemActive()) {
    auto *input = Input::GetInstance();
    const float dx = float(input->TriggerKey(DIK_RIGHT)) -
                     float(input->TriggerKey(DIK_LEFT));
    const float dy =
        float(input->TriggerKey(DIK_DOWN)) - float(input->TriggerKey(DIK_UP));
    if (dx || dy) {
      RememberUiEdit();
      auto &p = uiLayout_.elements[size_t(uiSelection_)].position;
      p[0] = (std::clamp)(p[0] + dx * (io.KeyShift ? 8 : 1), -4096.0f, 4096.0f);
      p[1] = (std::clamp)(p[1] + dy * (io.KeyShift ? 8 : 1), -4096.0f, 4096.0f);
    }
  }
  const auto currentDraws = uiLayout_.Resolve(UiContext(true));
  auto *overlay = ImGui::GetForegroundDrawList();
  overlay->AddRect({10, 104}, {910, 610.25f}, IM_COL32(60, 110, 140, 255));
  for (const auto &d : currentDraws)
    if (int(d.element) == uiSelection_) {
      ImVec2 a{10 + d.x * scale, 104 + d.y * scale},
          b{a.x + d.width * scale, a.y + d.height * scale};
      overlay->AddRect(a, b, IM_COL32(70, 230, 255, 255), 0, 0, 2);
      overlay->AddRectFilled({b.x - 5, b.y - 5}, {b.x + 5, b.y + 5},
                             IM_COL32(70, 230, 255, 255));
    }
#endif
}
