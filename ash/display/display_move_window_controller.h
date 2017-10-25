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

class ASH_EXPORT DisplayMoveWindowController {
 public:
  DisplayMoveWindowController();
  ~DisplayMoveWindowController();

  void HandleMoveWindowToLeftDisplay();
  void HandleMoveWindowToRightDisplay();
  void HandleMoveWindowToUpDisplay();
  void HandleMoveWindowToDownDisplay();

 private:
  int64_t GetNextLeft(const display::Display& origin_display);
  int64_t GetNextRight(const display::Display& origin_display);
  int64_t GetNextUp(const display::Display& origin_display);
  int64_t GetNextDown(const display::Display& origin_display);

  DISALLOW_COPY_AND_ASSIGN(DisplayMoveWindowController);
};

}  // namespace ash

#endif  // ASH_DISPLAY_DISPLAY_MOVE_WINDOW_CONTROLLER_H_
