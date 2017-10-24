// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/display_move_window_controller.h"

#include "ash/shell.h"
#include "ash/wm/window_util.h"
#include "ui/aura/window.h"
#include "ui/display/display.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/manager/managed_display_info.h"
#include "ui/display/screen.h"
#include "ui/display/types/display_constants.h"

namespace ash {

namespace {

bool IsValidDisplay(const display::ManagedDisplayInfo& display_info) {
  return display_info.id() != display::kInvalidDisplayId;
}

int GetVerticalOffset(const display::ManagedDisplayInfo& display1_info,
                      const display::ManagedDisplayInfo& display2_info) {
  return std::abs(display1_info.bounds_in_native().CenterPoint().y() -
                  display2_info.bounds_in_native().CenterPoint().y());
}

int GetHorizontalOffset(const display::ManagedDisplayInfo& display1_info,
                        const display::ManagedDisplayInfo& display2_info) {
  return std::abs(display1_info.bounds_in_native().CenterPoint().x() -
                  display2_info.bounds_in_native().CenterPoint().x());
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
  int64_t dest_display_id = GetNextLeft(
      Shell::Get()->display_manager()->GetDisplayInfo(origin_display.id()));
  wm::MoveWindowToDisplay(window, dest_display_id);
}

void DisplayMoveWindowController::HandleMoveWindowToRightDisplay() {
  aura::Window* window = wm::GetActiveWindow();
  if (!window)
    return;
  display::Display origin_display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(window);
  int64_t dest_display_id = GetNextRight(
      Shell::Get()->display_manager()->GetDisplayInfo(origin_display.id()));
  wm::MoveWindowToDisplay(window, dest_display_id);
}

void DisplayMoveWindowController::HandleMoveWindowToUpDisplay() {
  aura::Window* window = wm::GetActiveWindow();
  if (!window)
    return;
  display::Display origin_display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(window);
  int64_t dest_display_id = GetNextUp(
      Shell::Get()->display_manager()->GetDisplayInfo(origin_display.id()));
  wm::MoveWindowToDisplay(window, dest_display_id);
}

void DisplayMoveWindowController::HandleMoveWindowToDownDisplay() {
  aura::Window* window = wm::GetActiveWindow();
  if (!window)
    return;
  display::Display origin_display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(window);
  int64_t dest_display_id = GetNextDown(
      Shell::Get()->display_manager()->GetDisplayInfo(origin_display.id()));
  wm::MoveWindowToDisplay(window, dest_display_id);
}

int64_t DisplayMoveWindowController::GetNextLeft(
    const display::ManagedDisplayInfo& origin_display_info) {
  display::ManagedDisplayInfo dest_display_info[2];
  for (const auto& display : display::Screen::GetScreen()->GetAllDisplays()) {
    if (display.id() == origin_display_info.id())
      continue;
    const display::ManagedDisplayInfo display_info =
        Shell::Get()->display_manager()->GetDisplayInfo(display.id());
    if (GetHorizontalOffset(origin_display_info, display_info) == 0)
      continue;
    int candidate_left_space = origin_display_info.bounds_in_native().x() -
                               display_info.bounds_in_native().right();
    int candidate_vertical_offset =
        GetVerticalOffset(origin_display_info, display_info);
    int idx = candidate_left_space >= 0 ? 0 : 1;
    if (IsValidDisplay(dest_display_info[idx])) {
      int left_space = origin_display_info.bounds_in_native().x() -
                       dest_display_info[idx].bounds_in_native().right();
      if (candidate_left_space < left_space) {
        dest_display_info[idx] = display_info;
      } else if (candidate_left_space == left_space) {
        int vertical_offset =
            GetVerticalOffset(origin_display_info, dest_display_info[idx]);
        if (candidate_vertical_offset < vertical_offset)
          dest_display_info[idx] = display_info;
      }
    } else {
      dest_display_info[idx] = display_info;
    }
  }

  return IsValidDisplay(dest_display_info[0]) ? dest_display_info[0].id()
                                              : dest_display_info[1].id();
}

int64_t DisplayMoveWindowController::GetNextRight(
    const display::ManagedDisplayInfo& origin_display_info) {
  display::ManagedDisplayInfo dest_display_info[2];
  for (const auto& display : display::Screen::GetScreen()->GetAllDisplays()) {
    if (display.id() == origin_display_info.id())
      continue;
    const display::ManagedDisplayInfo display_info =
        Shell::Get()->display_manager()->GetDisplayInfo(display.id());
    if (GetHorizontalOffset(origin_display_info, display_info) == 0)
      continue;
    int candidate_right_space = display_info.bounds_in_native().x() -
                                origin_display_info.bounds_in_native().right();
    int candidate_vertical_offset =
        GetVerticalOffset(origin_display_info, display_info);
    int idx = candidate_right_space >= 0 ? 0 : 1;
    if (IsValidDisplay(dest_display_info[idx])) {
      int right_space = dest_display_info[idx].bounds_in_native().x() -
                        origin_display_info.bounds_in_native().right();
      if (candidate_right_space < right_space) {
        dest_display_info[idx] = display_info;
      } else if (candidate_right_space == right_space) {
        int vertical_offset =
            GetVerticalOffset(origin_display_info, dest_display_info[idx]);
        if (candidate_vertical_offset < vertical_offset)
          dest_display_info[idx] = display_info;
      }
    } else {
      dest_display_info[idx] = display_info;
    }
  }

  return IsValidDisplay(dest_display_info[0]) ? dest_display_info[0].id()
                                              : dest_display_info[1].id();
}

int64_t DisplayMoveWindowController::GetNextUp(
    const display::ManagedDisplayInfo& origin_display_info) {
  display::ManagedDisplayInfo dest_display_info[2];
  for (const auto& display : display::Screen::GetScreen()->GetAllDisplays()) {
    if (display.id() == origin_display_info.id())
      continue;
    const display::ManagedDisplayInfo display_info =
        Shell::Get()->display_manager()->GetDisplayInfo(display.id());
    if (GetVerticalOffset(origin_display_info, display_info) == 0)
      continue;
    int candidate_up_space = display_info.bounds_in_native().bottom() -
                             origin_display_info.bounds_in_native().y();
    int candidate_horizontal_offset =
        GetHorizontalOffset(origin_display_info, display_info);
    int idx = candidate_up_space >= 0 ? 0 : 1;
    if (IsValidDisplay(dest_display_info[idx])) {
      int up_space = dest_display_info[idx].bounds_in_native().bottom() -
                     origin_display_info.bounds_in_native().y();
      if (candidate_up_space < up_space) {
        dest_display_info[idx] = display_info;
      } else if (candidate_up_space == up_space) {
        int horizontal_offset =
            GetHorizontalOffset(origin_display_info, dest_display_info[idx]);
        if (candidate_horizontal_offset < horizontal_offset)
          dest_display_info[idx] = display_info;
      }
    } else {
      dest_display_info[idx] = display_info;
    }
  }

  return IsValidDisplay(dest_display_info[0]) ? dest_display_info[0].id()
                                              : dest_display_info[1].id();
}

int64_t DisplayMoveWindowController::GetNextDown(
    const display::ManagedDisplayInfo& origin_display_info) {
  display::ManagedDisplayInfo dest_display_info[2];
  for (const auto& display : display::Screen::GetScreen()->GetAllDisplays()) {
    if (display.id() == origin_display_info.id())
      continue;
    const display::ManagedDisplayInfo display_info =
        Shell::Get()->display_manager()->GetDisplayInfo(display.id());
    if (GetVerticalOffset(origin_display_info, display_info) == 0)
      continue;
    int candidate_down_space = origin_display_info.bounds_in_native().bottom() -
                               display_info.bounds_in_native().y();
    int candidate_horizontal_offset =
        GetHorizontalOffset(origin_display_info, display_info);
    int idx = candidate_down_space >= 0 ? 0 : 1;
    if (IsValidDisplay(dest_display_info[idx])) {
      int down_space = origin_display_info.bounds_in_native().bottom() -
                       dest_display_info[idx].bounds_in_native().y();
      if (candidate_down_space < down_space) {
        dest_display_info[idx] = display_info;
      } else if (candidate_down_space == down_space) {
        int horizontal_offset =
            GetHorizontalOffset(origin_display_info, dest_display_info[idx]);
        if (candidate_horizontal_offset < horizontal_offset)
          dest_display_info[idx] = display_info;
      }
    } else {
      dest_display_info[idx] = display_info;
    }
  }

  return IsValidDisplay(dest_display_info[0]) ? dest_display_info[0].id()
                                              : dest_display_info[1].id();
}

}  // namespace ash
