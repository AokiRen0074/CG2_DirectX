#include "SpearPrototype.h"
#include <algorithm>
#include <cmath>
#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void SpearPrototype::RememberStageEdit() {
  if (stageUndo_.size() >= 64)
    stageUndo_.erase(stageUndo_.begin());
  stageUndo_.push_back(stageDraft_);
  stageRedo_.clear();
}

void SpearPrototype::DrawStageEditor() {
#ifdef USE_IMGUI
  ImGui::SetNextWindowPos({20, 20}, fixedPreviewStep_ ? ImGuiCond_Always
                                                      : ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize({1240, 680}, fixedPreviewStep_
                                            ? ImGuiCond_Always
                                            : ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Stage editor - F3", &stageEditorOpen_)) {
    ImGui::End();
    return;
  }
  const SpearStage before = stageDraft_;
  bool changed = false, recorded = false;
  const auto resetSelection = [&] {
    stageSelectionType_ = 0;
    stageSelection_ = 0;
    stageMapDragging_ = false;
  };
  if (ImGui::Button("Save"))
    SaveStage();
  ImGui::SameLine();
  if (ImGui::Button("Load")) {
    if (stageDirty_)
      ImGui::OpenPopup("Load saved stage?");
    else {
      RememberStageEdit();
      LoadStage();
      resetSelection();
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Built-in tutorial"))
    ImGui::OpenPopup("Reset stage draft?");
  ImGui::SameLine();
  if (ImGui::Button("Undo") && !stageUndo_.empty()) {
    stageRedo_.push_back(stageDraft_);
    stageDraft_ = stageUndo_.back();
    stageUndo_.pop_back();
    stageDirty_ = true;
    resetSelection();
  }
  ImGui::SameLine();
  if (ImGui::Button("Redo") && !stageRedo_.empty()) {
    stageUndo_.push_back(stageDraft_);
    stageDraft_ = stageRedo_.back();
    stageRedo_.pop_back();
    stageDirty_ = true;
    resetSelection();
  }
  ImGui::SameLine();
  if (ImGui::Button("Play from start"))
    PlayStage();
  ImGui::SameLine();
  if (ImGui::Button("Test boss")) {
    try {
      stageDraft_.Validate();
      adventure_.RestartStage(stageDraft_);
      adventure_.StartBossPractice();
      flow_.Reset();
      effects_ = {};
      pendingJump_ = jumpHeld_ = false;
      accumulator_ = 0;
      stageEditorOpen_ = false;
    } catch (const std::exception &e) {
      stageStatus_ = e.what();
    }
  }
  ImGui::SameLine();
  ImGui::TextUnformatted(stageDirty_ ? "* Unsaved" : "Saved / unchanged");
  if (ImGui::BeginPopupModal("Load saved stage?", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextUnformatted("Replace unsaved changes with the saved stage?");
    if (ImGui::Button("Load saved")) {
      RememberStageEdit();
      LoadStage();
      resetSelection();
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
      ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
  }
  if (ImGui::BeginPopupModal("Reset stage draft?", nullptr,
                             ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::TextUnformatted(
        "Restore the original tutorial draft? Save writes it to disk.");
    if (ImGui::Button("Reset draft")) {
      RememberStageEdit();
      stageDraft_ = {};
      stageDirty_ = true;
      resetSelection();
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
      ImGui::CloseCurrentPopup();
    ImGui::EndPopup();
  }
  ImGui::TextWrapped("%s", stageStatus_.c_str());
  ImGui::TextUnformatted("Click to place / select. Drag to move. Wheel: zoom. "
                         "Middle drag: pan. Edits apply when you press Play.");
  const char *tools[] = {"Select", "Place floor", "Place enemy",
                         "Place checkpoint"};
  for (int i = 0; i < 4; ++i) {
    if (i)
      ImGui::SameLine();
    ImGui::RadioButton(tools[i], &stageTool_, i);
  }
  ImGui::SameLine();
  ImGui::Checkbox("Snap", &stageSnap_);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(70);
  ImGui::DragFloat("Grid", &stageGrid_, .25f, .25f, 4, "%.2f",
                   ImGuiSliderFlags_AlwaysClamp);
  ImGui::SameLine();
  const bool fit = ImGui::Button("Fit stage");
  const ImVec2 origin = ImGui::GetCursorScreenPos();
  const ImVec2 size((std::max)(ImGui::GetContentRegionAvail().x, 200.0f), 285);
  if (fit) {
    float left = stageDraft_.WorldLeft(), right = stageDraft_.WorldRight(),
          bottom = -8, top = 12;
    for (const auto &p : stageDraft_.platforms) {
      bottom = (std::min)(bottom, p.top);
      top = (std::max)(top, p.top);
    }
    stageMapX_ = (left + right) * .5f;
    stageMapY_ = (bottom + top) * .5f;
    stageMapWidth_ =
        (std::max)(right - left + 12, (top - bottom + 8) * size.x / size.y);
  }
  ImGui::InvisibleButton("Stage map", size,
                         ImGuiButtonFlags_MouseButtonLeft |
                             ImGuiButtonFlags_MouseButtonMiddle);
  const bool hovered = ImGui::IsItemHovered(), active = ImGui::IsItemActive();
  const auto &io = ImGui::GetIO();
  if (hovered && io.MouseWheel != 0) {
    const float old = stageMapWidth_;
    stageMapWidth_ =
        (std::clamp)(old * std::pow(.85f, io.MouseWheel), 20.0f, 2400.0f);
    stageMapX_ += (io.MousePos.x - origin.x - size.x * .5f) *
                  (old - stageMapWidth_) / size.x;
    stageMapY_ -= (io.MousePos.y - origin.y - size.y * .5f) *
                  (old - stageMapWidth_) / size.x;
  }
  const float scale = size.x / stageMapWidth_;
  if ((hovered || active) && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
    stageMapX_ -= io.MouseDelta.x / scale;
    stageMapY_ += io.MouseDelta.y / scale;
  }
  const auto screen = [&](float x, float y) {
    return ImVec2(origin.x + size.x * .5f + (x - stageMapX_) * scale,
                  origin.y + size.y * .5f - (y - stageMapY_) * scale);
  };
  const auto snap = [&](float x) {
    return stageSnap_ ? std::round(x / stageGrid_) * stageGrid_ : x;
  };
  const float mouseX =
      snap(stageMapX_ + (io.MousePos.x - origin.x - size.x * .5f) / scale);
  const float mouseY =
      snap(stageMapY_ - (io.MousePos.y - origin.y - size.y * .5f) / scale);
  const auto place = [&](int tool, float x, float y) {
    if (tool == 1 && stageDraft_.platforms.size() < SpearStage::maxPlatforms) {
      x = (std::clamp)(x, -253.0f, 2045.0f);
      y = (std::clamp)(y, -8.0f, 10.0f);
      stageDraft_.platforms.push_back({x - 3, x + 3, y});
      stageSelectionType_ = 0;
      stageSelection_ = int(stageDraft_.platforms.size()) - 1;
      changed = true;
    } else if (tool == 2 &&
               stageDraft_.enemies.size() < SpearStage::maxEnemies) {
      x = (std::clamp)(x, -255.0f, 2046.0f);
      y = (std::clamp)(y, -7.0f, 11.0f);
      stageDraft_.enemies.push_back(
          {x, y, 0, true, x + 2, false});
      stageDraft_.lessons.push_back("");
      stageSelectionType_ = 1;
      stageSelection_ = int(stageDraft_.enemies.size()) - 1;
      changed = true;
    } else if (tool == 3 && stageDraft_.spawns.size() < SpearStage::maxSpawns) {
      if (x <= stageDraft_.spawns.front().x) {
        stageStatus_ = "Checkpoint must be after START. Move START to change "
                       "the starting position.";
        return;
      }
      const float requestedY = y;
      float best = 10000;
      bool found = false;
      for (const auto &p : stageDraft_.platforms)
        if (x >= p.left + .5f && x <= p.right - .5f &&
            std::abs(requestedY - p.top) < best) {
          best = std::abs(requestedY - p.top);
          y = p.top;
          found = true;
        }
      if (!found) {
        stageStatus_ = "Place the checkpoint over a floor.";
        return;
      }
      auto it =
          std::lower_bound(stageDraft_.spawns.begin(), stageDraft_.spawns.end(),
                           x, [](const auto &p, float v) { return p.x < v; });
      stageSelection_ = int(it - stageDraft_.spawns.begin());
      stageDraft_.spawns.insert(it, {x, y});
      stageSelectionType_ = 2;
      changed = true;
    }
  };
  if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    stageMapDragging_ = false;
    stageDragRecorded_ = false;
    stageDragHandle_ = 0;
    stageDragX_ = mouseX;
    stageDragY_ = mouseY;
    if (stageTool_ != 0)
      place(stageTool_, mouseX, mouseY);
    else {
      float best = 14 * 14.0f;
      const auto pick = [&](ImVec2 p, int type, int index, int handle = 0) {
        const float dx = io.MousePos.x - p.x, dy = io.MousePos.y - p.y,
                    d = dx * dx + dy * dy;
        if (d < best) {
          best = d;
          stageSelectionType_ = type;
          stageSelection_ = index;
          stageDragHandle_ = handle;
          stageMapDragging_ = true;
        }
      };
      for (size_t i = 0; i < stageDraft_.platforms.size(); ++i) {
        const auto &p = stageDraft_.platforms[i];
        const auto a = screen(p.left, p.top), b = screen(p.right, p.top);
        pick({(std::clamp)(io.MousePos.x, a.x, b.x), a.y}, 0, int(i));
      }
      if (stageSelectionType_ == 0 &&
          size_t(stageSelection_) < stageDraft_.platforms.size() &&
          size_t(stageSelection_) != stageDraft_.arenaPlatform) {
        const auto &p = stageDraft_.platforms[size_t(stageSelection_)];
        pick(screen(p.left, p.top), 0, stageSelection_, 1);
        pick(screen(p.right, p.top), 0, stageSelection_, 2);
      }
      for (size_t i = 0; i < stageDraft_.enemies.size(); ++i)
        pick(screen(stageDraft_.enemies[i].x, stageDraft_.enemies[i].y), 1,
             int(i));
      for (size_t i = 0; i < stageDraft_.spawns.size(); ++i) {
        auto p = screen(stageDraft_.spawns[i].x, stageDraft_.spawns[i].y);
        p.y -= 16;
        pick(p, 2, int(i));
      }
      const auto p = screen(stageDraft_.bossStart, 9);
      pick({p.x,
            (std::clamp)(io.MousePos.y, origin.y + 14, origin.y + size.y - 14)},
           3, 0);
    }
  }
  if (stageMapDragging_ && active &&
      ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    const float dx = mouseX - stageDragX_, dy = mouseY - stageDragY_;
    if ((dx != 0 || dy != 0) && !stageDragRecorded_) {
      RememberStageEdit();
      stageDragRecorded_ = true;
    }
    recorded = true;
    if (stageSelectionType_ == 0) {
      auto &p = stageDraft_.platforms[size_t(stageSelection_)];
      if (stageDragHandle_ == 1)
        p.left = (std::clamp)(p.left + dx, -256.0f, p.right - 1);
      else if (stageDragHandle_ == 2)
        p.right = (std::clamp)(p.right + dx, p.left + 1, 2048.0f);
      else if (size_t(stageSelection_) == stageDraft_.arenaPlatform)
        stageDraft_.SetBossStart(
            (std::clamp)(stageDraft_.bossStart + dx, -128.0f, 2000.0f));
      else
        stageDraft_.MovePlatform(
            size_t(stageSelection_),
            (std::clamp)(dx, -256 - p.left, 2048 - p.right),
            (std::clamp)(dy, -8 - p.top, 10 - p.top));
    } else if (stageSelectionType_ == 1) {
      auto &e = stageDraft_.enemies[size_t(stageSelection_)];
      const float old = e.x;
      e.x = (std::clamp)(e.x + dx, -255.0f, 2046.0f);
      e.y = (std::clamp)(e.y + dy, -7.0f, 11.0f);
      e.gate += (e.x - old);
    } else if (stageSelectionType_ == 2) {
      auto &p = stageDraft_.spawns[size_t(stageSelection_)];
      p.x = (std::clamp)(p.x + dx, -255.0f, 2047.0f);
      p.y = (std::clamp)(p.y + dy, -8.0f, 10.0f);
      for (const auto &floor : stageDraft_.platforms)
        if (p.x >= floor.left + .5f && p.x <= floor.right - .5f &&
            std::abs(p.y - floor.top) < .75f)
          p.y = floor.top;
    } else
      stageDraft_.SetBossStart(
          (std::clamp)(stageDraft_.bossStart + dx, -128.0f, 2000.0f));
    stageDragX_ = mouseX;
    stageDragY_ = mouseY;
    changed |= dx != 0 || dy != 0;
  }
  if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
    stageMapDragging_ = false;
  auto *draw = ImGui::GetWindowDrawList();
  draw->PushClipRect(origin, {origin.x + size.x, origin.y + size.y}, true);
  draw->AddRectFilled(origin, {origin.x + size.x, origin.y + size.y},
                      IM_COL32(19, 26, 36, 255));
  const float step = stageMapWidth_ > 600   ? 100.0f
                     : stageMapWidth_ > 200 ? 20.0f
                     : stageMapWidth_ > 60  ? 10.0f
                                            : 2.0f;
  for (float x = std::floor((stageMapX_ - stageMapWidth_ * .5f) / step) * step;
       x < stageMapX_ + stageMapWidth_ * .5f; x += step) {
    const auto p = screen(x, stageMapY_);
    draw->AddLine({p.x, origin.y}, {p.x, origin.y + size.y},
                  IM_COL32(45, 55, 65, 255));
    draw->AddText({p.x + 3, origin.y + 2}, IM_COL32(130, 150, 170, 255),
                  std::to_string(int(x)).c_str());
  }
  const auto &arena = stageDraft_.Arena();
  draw->AddRectFilled(screen(arena.left, 12), screen(arena.right, -8),
                      IM_COL32(65, 45, 50, 120));
  for (size_t i = 0; i < stageDraft_.platforms.size(); ++i) {
    const auto &p = stageDraft_.platforms[i];
    const bool selected = stageSelectionType_ == 0 && stageSelection_ == int(i);
    const auto a = screen(p.left, p.top), b = screen(p.right, p.top);
    draw->AddLine(a, b,
                  selected ? IM_COL32(255, 220, 100, 255)
                           : IM_COL32(80, 195, 225, 255),
                  6);
    draw->AddText({a.x, a.y + 6}, IM_COL32(195, 215, 235, 255),
                  ("P" + std::to_string(i)).c_str());
    if (selected && i != stageDraft_.arenaPlatform) {
      draw->AddCircleFilled(a, 5, IM_COL32(255, 220, 100, 255));
      draw->AddCircleFilled(b, 5, IM_COL32(255, 220, 100, 255));
    }
  }
  for (size_t i = 0; i < stageDraft_.enemies.size(); ++i) {
    const auto &e = stageDraft_.enemies[i];
    const auto p = screen(e.x, e.y);
    const bool selected = stageSelectionType_ == 1 && stageSelection_ == int(i);
    draw->AddCircleFilled(p, selected ? 8.0f : 6.0f,
                          e.enabled ? IM_COL32(255, 110, 100, 255)
                                    : IM_COL32(100, 100, 100, 255));
    draw->AddText({p.x + 8, p.y - 8}, IM_COL32(255, 210, 200, 255),
                  ("E" + std::to_string(i)).c_str());
    if (selected && e.blocksPath) {
      const auto gate = screen(e.gate, 0);
      draw->AddLine({gate.x, origin.y}, {gate.x, origin.y + size.y},
                    IM_COL32(255, 190, 80, 180));
    }
  }
  for (size_t i = 0; i < stageDraft_.spawns.size(); ++i) {
    const auto p = screen(stageDraft_.spawns[i].x, stageDraft_.spawns[i].y);
    draw->AddLine(p, {p.x, p.y - 22}, IM_COL32(100, 255, 150, 255), 3);
    draw->AddText({p.x + 3, p.y - 30}, IM_COL32(100, 255, 150, 255),
                  (i == 0 ? "START" : "C" + std::to_string(i)).c_str());
  }
  const auto boss = screen(stageDraft_.bossStart, 9);
  draw->AddLine({boss.x, origin.y}, {boss.x, origin.y + size.y},
                IM_COL32(255, 170, 70, 255), 2);
  draw->AddText({boss.x + 5, origin.y + 19}, IM_COL32(255, 190, 100, 255),
                "BOSS START");
  if (hovered && stageTool_ != 0) {
    const auto p = screen(mouseX, mouseY);
    draw->AddCircle(p, 8, IM_COL32(255, 255, 255, 200));
  }
  draw->PopClipRect();

  ImGui::Text("Floors %zu / %zu   Enemies %zu / %zu   Checkpoints %zu / %zu",
              stageDraft_.platforms.size(), SpearStage::maxPlatforms,
              stageDraft_.enemies.size(), SpearStage::maxEnemies,
              stageDraft_.spawns.size(), SpearStage::maxSpawns);
  ImGui::PushItemWidth(180);
  const char *types[] = {"Floor", "Enemy", "Start / checkpoint", "Boss start"};
  if (ImGui::Combo("Selection", &stageSelectionType_, types, 4))
    stageSelection_ = 0;
  const int count = stageSelectionType_ == 0 ? int(stageDraft_.platforms.size())
                    : stageSelectionType_ == 1 ? int(stageDraft_.enemies.size())
                    : stageSelectionType_ == 2 ? int(stageDraft_.spawns.size())
                                               : 1;
  stageSelection_ = (std::clamp)(stageSelection_, 0, (std::max)(0, count - 1));
  ImGui::SameLine();
  if (count > 0)
    ImGui::SliderInt("Index", &stageSelection_, 0, count - 1);
  const auto edit = [](const char *name, float &v, float lo, float hi) {
    return ImGui::DragFloat(name, &v, .05f, lo, hi, "%.2f",
                            ImGuiSliderFlags_AlwaysClamp);
  };
  if (stageSelectionType_ == 0 && count > 0) {
    const size_t i = size_t(stageSelection_);
    auto &p = stageDraft_.platforms[i];
    if (i == stageDraft_.arenaPlatform)
      ImGui::TextUnformatted(
          "Boss floor follows BOSS START. Select the boss marker to move it.");
    else {
      float x = (p.left + p.right) * .5f, y = p.top, width = p.right - p.left;
      const bool moved =
          edit("Floor center X", x, -256 + width * .5f, 2048 - width * .5f);
      ImGui::SameLine();
      const bool raised = edit("Floor top Y", y, -8, 10);
      if (moved || raised) {
        stageDraft_.MovePlatform(i, x - (p.left + p.right) * .5f, y - p.top);
        changed = true;
      }
      if (edit("Floor width", width, 1,
               (std::min)(2 * (x + 256), 2 * (2048 - x)))) {
        p.left = x - width * .5f;
        p.right = x + width * .5f;
        changed = true;
      }
    }
  } else if (stageSelectionType_ == 1 && count > 0) {
    auto &e = stageDraft_.enemies[size_t(stageSelection_)];
    changed |= edit("Enemy X", e.x, -255, 2046);
    ImGui::SameLine();
    changed |= edit("Enemy Y", e.y, -7, 11);
    ImGui::TextUnformatted("Enemy: defeat to restore +1 air jump");
    e.kind=0;
    changed |= ImGui::Checkbox("Enabled", &e.enabled);
    changed |= ImGui::Checkbox("Blocks path until defeated", &e.blocksPath);
    ImGui::SameLine();
    if (e.blocksPath)
      changed |= edit("Gate X", e.gate, e.x + 1, 2048);
  } else if (stageSelectionType_ == 2 && count > 0) {
    auto &p = stageDraft_.spawns[size_t(stageSelection_)];
    changed |= edit("Spawn X", p.x, -255, 2047);
    ImGui::SameLine();
    changed |= edit("Spawn Y", p.y, -8, 10);
    ImGui::SameLine();
    if (ImGui::Button("Snap to floor")) {
      const float originalY = p.y;
      float best = 10000;
      for (const auto &f : stageDraft_.platforms)
        if (p.x >= f.left + .5f && p.x <= f.right - .5f &&
            std::abs(originalY - f.top) < best) {
          best = std::abs(originalY - f.top);
          p.y = f.top;
          changed = true;
        }
    }
  } else if (stageSelectionType_ == 3) {
    float x = stageDraft_.bossStart;
    if (edit("Boss start X", x, -128, 2000)) {
      stageDraft_.SetBossStart(x);
      changed = true;
    }
    ImGui::SameLine();
    changed |= ImGui::Checkbox("Boss enabled", &stageDraft_.bossEnabled);
    ImGui::TextUnformatted(
        "Moving the marker moves the arena, boss attacks, camera and boss "
        "checkpoint together. Off: this marker is the finish.");
  }
  const bool protectedItem =
      (stageSelectionType_ == 0 &&
       size_t(stageSelection_) == stageDraft_.arenaPlatform) ||
      (stageSelectionType_ == 2 && stageSelection_ == 0) ||
      stageSelectionType_ == 3;
  if (count > 0 && !protectedItem) {
    if (ImGui::Button("Duplicate")) {
      if (stageSelectionType_ == 0 &&
          stageDraft_.platforms.size() < SpearStage::maxPlatforms) {
        auto p = stageDraft_.platforms[size_t(stageSelection_)];
        const float dx = (std::min)(2.0f, 2048 - p.right);
        p.left += dx;
        p.right += dx;
        stageDraft_.platforms.push_back(p);
        stageSelection_ = int(stageDraft_.platforms.size()) - 1;
        changed = true;
      } else if (stageSelectionType_ == 1 &&
                 stageDraft_.enemies.size() < SpearStage::maxEnemies) {
        auto e = stageDraft_.enemies[size_t(stageSelection_)];
        e.x = (std::min)(e.x + 2, 2046.0f);
        e.gate = (std::min)(e.gate + 2, 2048.0f);
        const auto lesson = stageDraft_.lessons[size_t(stageSelection_)];
        stageDraft_.enemies.push_back(e);
        stageDraft_.lessons.push_back(lesson);
        stageSelection_ = int(stageDraft_.enemies.size()) - 1;
        changed = true;
      } else if (stageSelectionType_ == 2) {
        const auto p = stageDraft_.spawns[size_t(stageSelection_)];
        place(3, p.x + 1, p.y);
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete selected")) {
      if (stageSelectionType_ == 0)
        changed |= stageDraft_.RemovePlatform(size_t(stageSelection_));
      else if (stageSelectionType_ == 1)
        changed |= stageDraft_.RemoveEnemy(size_t(stageSelection_));
      else if (stageSelectionType_ == 2) {
        stageDraft_.spawns.erase(stageDraft_.spawns.begin() + stageSelection_);
        changed = true;
      }
      stageSelection_ = 0;
    }
  }
  ImGui::PopItemWidth();
  if (changed) {
    if (!recorded) {
      if (stageUndo_.size() >= 64)
        stageUndo_.erase(stageUndo_.begin());
      stageUndo_.push_back(before);
      stageRedo_.clear();
    }
    stageDirty_ = true;
  }
  try {
    stageDraft_.Validate();
    ImGui::TextColored({.5f, 1, .6f, 1}, "Ready to save / play");
  } catch (const std::exception &e) {
    ImGui::TextWrapped("Fix before save / play: %s", e.what());
  }
  ImGui::End();
#endif
}
