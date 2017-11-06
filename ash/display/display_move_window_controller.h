// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_DISPLAY_DISPLAY_MOVE_WINDOW_CONTROLLER_H_
#define ASH_DISPLAY_DISPLAY_MOVE_WINDOW_CONTROLLER_H_

#include <stdint.h>

#include "ash/ash_export.h"
#include "base/macros.h"

namespace display {
class Display;
}  // namespace display

namespace ash {

// Handles moving current active window from its display to another display
// specified by |Direction|.
class ASH_EXPORT DisplayMoveWindowController {
 public:
  enum class Direction { kLeft, kUp, kRight, kDown };

  DisplayMoveWindowController() = default;
  ~DisplayMoveWindowController() = default;

  void HandleMoveWindowToDisplay(Direction direction);

 private:
  // Two display candidate types when getting next display.
  enum class DisplayCandidateType {
    // Indicates candidate display is in the specified movement direction.
    kInMovementDirection = 0,

    // Indicates candidate didplsy is in the cycled movement direction.
    kInCycledDirection = 1
  };

  // Gets the display id of next display of |origin_display| on the moving
  // |direction|. Cycled direction result is returned if on moving |direction|
  // doesn't contain eligible candidate displays.
  int64_t GetNextDisplay(const display::Display& origin_display,
                         Direction direction) const;

  // Returns true if candidate display is not completely off the bounds of
  // original display in the moving direction or cycled direction.
  bool IsWithinBounds(const display::Display& candidate_display,
                      const display::Display& origin_display,
                      Direction direction) const;

  // Gets the space between displays. The calculation is based on |direction|.
  // Non-negative value indicates |kInMovementDirection| and negative value
  // indicates |kInCycledDirection|.
  int GetSpaceBetweenDisplays(const display::Display& candidate_display,
                              const display::Display& origin_display,
                              Direction direction) const;

  // Gets the orthogonal offset between displays, which is vertical offset for
  // left/right, horizontal offset for up/down.
  int GetOrthogonalOffsetBetweenDisplays(
      const display::Display& candidate_display,
      const display::Display& origin_display,
      Direction direction) const;

  DISALLOW_COPY_AND_ASSIGN(DisplayMoveWindowController);
};

}  // namespace ash

#endif  // ASH_DISPLAY_DISPLAY_MOVE_WINDOW_CONTROLLER_H_
