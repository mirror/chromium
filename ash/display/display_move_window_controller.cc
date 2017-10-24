// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/display_move_window_controller.h"

#include "ash/shell.h"
#include "ash/wm/window_util.h"
#include "ui/aura/window.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/display/types/display_constants.h"

#include "ui/display/manager/display_manager.h"
#include "ui/display/manager/managed_display_info.h"

namespace ash {

namespace {

int GetVerticalOffset(const display::Display& display1,
                      const display::Display& display2) {
  DCHECK(display1.is_valid());
  DCHECK(display2.is_valid());
  return std::abs(display1.bounds().CenterPoint().y() -
                  display2.bounds().CenterPoint().y());
}

}  // namespace

DisplayMoveWindowController::DisplayMoveWindowController() = default;

DisplayMoveWindowController::~DisplayMoveWindowController() = default;

void DisplayMoveWindowController::HandleMoveWindowToLeftDisplay() {
  aura::Window* window = wm::GetActiveWindow();
  if (!window)
    return;

  display::Display source_display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(window);
  int64_t target_display_id = GetNextLeft(source_display);
  wm::MoveWindowToDisplay(window, target_display_id);
}

void DisplayMoveWindowController::HandleMoveWindowToRightDisplay() {
  aura::Window* window = wm::GetActiveWindow();
  if (!window)
    return;
  display::Display source_display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(window);
  int64_t target_display_id = GetNextRight(source_display);
  wm::MoveWindowToDisplay(window, target_display_id);
}

int64_t DisplayMoveWindowController::GetNextLeft(
    const display::Display& source_display) {
  DCHECK(source_display.is_valid());
  display::Display target_display[2];
  for (const auto& display : display::Screen::GetScreen()->GetAllDisplays()) {
    if (display.id() == source_display.id())
      continue;
    const display::ManagedDisplayInfo display_info =
        Shell::Get()->display_manager()->GetDisplayInfo(display.id());
    int candidate_left_padding =
        source_display.bounds().x() - display.bounds().right();
    int candidate_vertical_offset = GetVerticalOffset(source_display, display);
    int idx = candidate_left_padding >= 0 ? 0 : 1;
    if (target_display[idx].is_valid()) {
      int left_padding =
          source_display.bounds().x() - target_display[idx].bounds().right();
      if (candidate_left_padding < left_padding) {
        target_display[idx] = display;
      } else if (candidate_left_padding == left_padding) {
        int vertical_offset =
            GetVerticalOffset(source_display, target_display[idx]);
        if (candidate_vertical_offset < vertical_offset)
          target_display[idx] = display;
      }
    } else {
      target_display[idx] = display;
    }
  }

  return target_display[0].is_valid() ? target_display[0].id()
                                      : target_display[1].id();
}

int64_t DisplayMoveWindowController::GetNextRight(
    const display::Display& source_display) {
  DCHECK(source_display.is_valid());
  display::Display target_display[2];
  for (const auto& display : display::Screen::GetScreen()->GetAllDisplays()) {
    if (display.id() == source_display.id())
      continue;
    int candidate_right_padding =
        display.bounds().x() - source_display.bounds().right();
    int candidate_vertical_offset = GetVerticalOffset(source_display, display);
    int idx = candidate_right_padding >= 0 ? 0 : 1;
    if (target_display[idx].is_valid()) {
      int right_padding =
          target_display[idx].bounds().x() - source_display.bounds().right();
      if (candidate_right_padding < right_padding) {
        target_display[idx] = display;
      } else if (candidate_right_padding == right_padding) {
        int vertical_offset =
            GetVerticalOffset(source_display, target_display[idx]);
        if (candidate_vertical_offset < vertical_offset)
          target_display[idx] = display;
      }
    } else {
      target_display[idx] = display;
    }
  }

  return target_display[0].is_valid() ? target_display[0].id()
                                      : target_display[1].id();
}

}  // namespace ash
