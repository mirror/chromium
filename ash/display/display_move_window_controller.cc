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

namespace ash {

namespace {

int GetVerticalOffset(const display::Display& display1,
                      const display::Display& display2) {
  DCHECK(display1.is_valid());
  DCHECK(display2.is_valid());
  return std::abs(display1.bounds().CenterPoint().y() -
                  display2.bounds().CenterPoint().y());
}

int GetHorizontalOffset(const display::Display& display1,
                        const display::Display& display2) {
  DCHECK(display1.is_valid());
  DCHECK(display2.is_valid());
  return std::abs(display1.bounds().CenterPoint().x() -
                  display2.bounds().CenterPoint().x());
}

}  // namespace

DisplayMoveWindowController::DisplayMoveWindowController() = default;

DisplayMoveWindowController::~DisplayMoveWindowController() = default;

void DisplayMoveWindowController::HandleMoveWindowToLeftDisplay() {
  aura::Window* window = wm::GetActiveWindow();
  if (!window)
    return;

  display::Display origin_display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(window);
  int64_t dest_display_id = GetNextLeft(origin_display);
  wm::MoveWindowToDisplay(window, dest_display_id);
}

void DisplayMoveWindowController::HandleMoveWindowToRightDisplay() {
  aura::Window* window = wm::GetActiveWindow();
  if (!window)
    return;
  display::Display origin_display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(window);
  int64_t dest_display_id = GetNextRight(origin_display);
  wm::MoveWindowToDisplay(window, dest_display_id);
}

void DisplayMoveWindowController::HandleMoveWindowToUpDisplay() {
  aura::Window* window = wm::GetActiveWindow();
  if (!window)
    return;
  display::Display origin_display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(window);
  int64_t dest_display_id = GetNextUp(origin_display);
  wm::MoveWindowToDisplay(window, dest_display_id);
}

void DisplayMoveWindowController::HandleMoveWindowToDownDisplay() {
  aura::Window* window = wm::GetActiveWindow();
  if (!window)
    return;
  display::Display origin_display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(window);
  int64_t dest_display_id = GetNextDown(origin_display);
  wm::MoveWindowToDisplay(window, dest_display_id);
}

int64_t DisplayMoveWindowController::GetNextLeft(
    const display::Display& origin_display) {
  display::Display dest_display[2];
  for (const auto& display : display::Screen::GetScreen()->GetAllDisplays()) {
    if (display.id() == origin_display.id())
      continue;
    if (GetHorizontalOffset(origin_display, display) == 0)
      continue;
    int candidate_left_space =
        origin_display.bounds().x() - display.bounds().right();
    int candidate_vertical_offset = GetVerticalOffset(origin_display, display);
    int idx = candidate_left_space >= 0 ? 0 : 1;
    if (dest_display[idx].is_valid()) {
      int left_space =
          origin_display.bounds().x() - dest_display[idx].bounds().right();
      if (candidate_left_space < left_space) {
        dest_display[idx] = display;
      } else if (candidate_left_space == left_space) {
        int vertical_offset =
            GetVerticalOffset(origin_display, dest_display[idx]);
        if (candidate_vertical_offset < vertical_offset)
          dest_display[idx] = display;
      }
    } else {
      dest_display[idx] = display;
    }
  }

  return dest_display[0].is_valid() ? dest_display[0].id()
                                    : dest_display[1].id();
}

int64_t DisplayMoveWindowController::GetNextRight(
    const display::Display& origin_display) {
  display::Display dest_display[2];
  for (const auto& display : display::Screen::GetScreen()->GetAllDisplays()) {
    if (display.id() == origin_display.id())
      continue;
    if (GetHorizontalOffset(origin_display, display) == 0)
      continue;
    int candidate_right_space =
        display.bounds().x() - origin_display.bounds().right();
    int candidate_vertical_offset = GetVerticalOffset(origin_display, display);
    int idx = candidate_right_space >= 0 ? 0 : 1;
    if (dest_display[idx].is_valid()) {
      int right_space =
          dest_display[idx].bounds().x() - origin_display.bounds().right();
      if (candidate_right_space < right_space) {
        dest_display[idx] = display;
      } else if (candidate_right_space == right_space) {
        int vertical_offset =
            GetVerticalOffset(origin_display, dest_display[idx]);
        if (candidate_vertical_offset < vertical_offset)
          dest_display[idx] = display;
      }
    } else {
      dest_display[idx] = display;
    }
  }

  return dest_display[0].is_valid() ? dest_display[0].id()
                                    : dest_display[1].id();
}

int64_t DisplayMoveWindowController::GetNextUp(
    const display::Display& origin_display) {
  display::Display dest_display[2];
  for (const auto& display : display::Screen::GetScreen()->GetAllDisplays()) {
    if (display.id() == origin_display.id())
      continue;
    if (GetVerticalOffset(origin_display, display) == 0)
      continue;
    int candidate_up_space =
        display.bounds().bottom() - origin_display.bounds().y();
    int candidate_horizontal_offset =
        GetHorizontalOffset(origin_display, display);
    int idx = candidate_up_space >= 0 ? 0 : 1;
    if (dest_display[idx].is_valid()) {
      int up_space =
          dest_display[idx].bounds().bottom() - origin_display.bounds().y();
      if (candidate_up_space < up_space) {
        dest_display[idx] = display;
      } else if (candidate_up_space == up_space) {
        int horizontal_offset =
            GetHorizontalOffset(origin_display, dest_display[idx]);
        if (candidate_horizontal_offset < horizontal_offset)
          dest_display[idx] = display;
      }
    } else {
      dest_display[idx] = display;
    }
  }

  return dest_display[0].is_valid() ? dest_display[0].id()
                                    : dest_display[1].id();
}

int64_t DisplayMoveWindowController::GetNextDown(
    const display::Display& origin_display) {
  display::Display dest_display[2];
  for (const auto& display : display::Screen::GetScreen()->GetAllDisplays()) {
    if (display.id() == origin_display.id())
      continue;
    if (GetVerticalOffset(origin_display, display) == 0)
      continue;
    int candidate_down_space =
        origin_display.bounds().bottom() - display.bounds().y();
    int candidate_horizontal_offset =
        GetHorizontalOffset(origin_display, display);
    int idx = candidate_down_space >= 0 ? 0 : 1;
    if (dest_display[idx].is_valid()) {
      int down_space =
          origin_display.bounds().bottom() - dest_display[idx].bounds().y();
      if (candidate_down_space < down_space) {
        dest_display[idx] = display;
      } else if (candidate_down_space == down_space) {
        int horizontal_offset =
            GetHorizontalOffset(origin_display, dest_display[idx]);
        if (candidate_horizontal_offset < horizontal_offset)
          dest_display[idx] = display;
      }
    } else {
      dest_display[idx] = display;
    }
  }

  return dest_display[0].is_valid() ? dest_display[0].id()
                                    : dest_display[1].id();
}

}  // namespace ash
