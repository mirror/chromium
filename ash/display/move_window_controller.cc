// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/move_window_controller.h"

#include "ash/wm/window_util.h"
#include "ui/aura/window.h"
#include "ui/display/screen.h"
#include "ui/display/types/display_constants.h"

namespace ash {

MoveWindowController::MoveWindowController() = default;

MoveWindowController::~MoveWindowController() = default;

void MoveWindowController::HandleMoveWindowToLeftDisplay() {
  aura::Window* active_window = wm::GetActiveWindow();
  if (!active_window)
    return;

  for (const auto& display : display::Screen::GetAllDisplays()) {}
}

int64_t MoveWindowController::GetNextLet(int64_t display_id) {
  return display::kInvalidDisplayId;
}

}  // namespace ash
