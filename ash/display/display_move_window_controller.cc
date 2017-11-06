// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/display_move_window_controller.h"

#include <array>

#include "ash/shell.h"
#include "ash/wm/window_util.h"
#include "ui/aura/window.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/display/types/display_constants.h"

namespace ash {

namespace {

// Calculate the vertical offset between two displays' center points.
int GetVerticalOffset(const display::Display& display1,
                      const display::Display& display2) {
  DCHECK(display1.is_valid());
  DCHECK(display2.is_valid());
  return std::abs(display1.bounds().CenterPoint().y() -
                  display2.bounds().CenterPoint().y());
}

// Calculate the horizontal offset between two displays' center points.
int GetHorizontalOffset(const display::Display& display1,
                        const display::Display& display2) {
  DCHECK(display1.is_valid());
  DCHECK(display2.is_valid());
  return std::abs(display1.bounds().CenterPoint().x() -
                  display2.bounds().CenterPoint().x());
}

}  // namespace

void DisplayMoveWindowController::HandleMoveWindowToDisplay(
    Direction direction) {
  aura::Window* window = wm::GetActiveWindow();
  if (!window)
    return;

  display::Display origin_display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(window);
  int64_t dest_display_id = GetNextDisplay(origin_display, direction);
  wm::MoveWindowToDisplay(window, dest_display_id);
}

int64_t DisplayMoveWindowController::GetNextDisplay(
    const display::Display& origin_display,
    Direction direction) const {
  std::array<display::Display, 2> dest_display;
  for (const auto& candidate_display :
       display::Screen::GetScreen()->GetAllDisplays()) {
    if (candidate_display.id() == origin_display.id())
      continue;
    // Ignore the candidate display if |IsWithinBounds| returns true. This
    // ensures that |candidate_space| calculated below represents the real
    // layout apart distance.
    if (IsWithinBounds(candidate_display, origin_display, direction))
      continue;
    const int candidate_space =
        GetSpaceBetweenDisplays(candidate_display, origin_display, direction);
    const int idx = static_cast<int>(
        candidate_space >= 0 ? DisplayCandidateType::kInMovementDirection
                             : DisplayCandidateType::kInCycledDirection);
    if (dest_display[idx].is_valid()) {
      const int current_space =
          GetSpaceBetweenDisplays(dest_display[idx], origin_display, direction);
      if (candidate_space < current_space) {
        dest_display[idx] = candidate_display;
      } else if (candidate_space == current_space) {
        if (GetOrthogonalOffsetBetweenDisplays(candidate_display,
                                               origin_display, direction) <
            GetOrthogonalOffsetBetweenDisplays(dest_display[idx],
                                               origin_display, direction)) {
          dest_display[idx] = candidate_display;
        }
      }
    } else {
      dest_display[idx] = candidate_display;
    }
  }

  return dest_display[0].is_valid() ? dest_display[0].id()
                                    : dest_display[1].id();
}

bool DisplayMoveWindowController::IsWithinBounds(
    const display::Display& candidate_display,
    const display::Display& origin_display,
    Direction direction) const {
  if (direction == Direction::kLeft || direction == Direction::kRight) {
    return GetHorizontalOffset(candidate_display, origin_display) <
           (candidate_display.bounds().width() +
            origin_display.bounds().width()) /
               2;
  }

  return GetVerticalOffset(candidate_display, origin_display) <
         (candidate_display.bounds().height() +
          origin_display.bounds().height()) /
             2;
}

int DisplayMoveWindowController::GetSpaceBetweenDisplays(
    const display::Display& candidate_display,
    const display::Display& origin_display,
    Direction direction) const {
  switch (direction) {
    case Direction::kLeft:
      return origin_display.bounds().x() - candidate_display.bounds().right();
    case Direction::kUp:
      return origin_display.bounds().y() - candidate_display.bounds().bottom();
    case Direction::kRight:
      return candidate_display.bounds().x() - origin_display.bounds().right();
    case Direction::kDown:
      return candidate_display.bounds().y() - origin_display.bounds().bottom();
  }
  NOTREACHED();
  return 0;
}

int DisplayMoveWindowController::GetOrthogonalOffsetBetweenDisplays(
    const display::Display& candidate_display,
    const display::Display& origin_display,
    Direction direction) const {
  if (direction == Direction::kLeft || direction == Direction::kRight)
    return GetVerticalOffset(candidate_display, origin_display);
  return GetHorizontalOffset(candidate_display, origin_display);
}

}  // namespace ash
