// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/persistent_window_controller.h"

#include "ash/shell.h"
#include "ash/wm/mru_window_tracker.h"
#include "ash/wm/window_state.h"

namespace ash {

namespace {

bool IsDisplayInList()

}  // namespace

PersistentWindowController::PersistentWindowController()
    : screen_(display::Screen::GetScreen()) {
  Shell::Get()->display_configurator()->AddObserver(this);
}

PersistentWindowController::~PersistentWindowController() {
  Shell::Get()->display_configurator()->RemoveObserver(this);
}

void PersistentWindowController::SavePersistentWindowBounds(
    const std::vector<Display>& displays) {
  MruWindowTracker::WindowList window_list =
      Shell::Get()->mru_window_tracker()->BuildWindowForCycleList();
  for (auto* window : window_list) {
  }
}

void PersistentWindowController::OnDisplayModeChanged(
    const display::DisplayConfigurator::DisplayStateList& displays) {}

}  // namespace ash
