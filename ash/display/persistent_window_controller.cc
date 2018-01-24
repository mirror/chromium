// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/persistent_window_controller.h"

#include "ash/shell.h"
#include "ash/wm/mru_window_tracker.h"
#include "ash/wm/window_state.h"
#include "ui/display/manager/display_manager.h"

namespace ash {

namespace {

display::DisplayManager* GetDisplayManager() {
  return Shell::Get()->display_manager();
}

}  // namespace

PersistentWindowController::PersistentWindowController()
    : screen_(display::Screen::GetScreen()) {
  Shell::Get()->display_configurator()->AddObserver(this);
}

PersistentWindowController::~PersistentWindowController() {
  Shell::Get()->display_configurator()->RemoveObserver(this);
}

void PersistentWindowController::SavePersistentWindowBounds(
    const std::vector<display::Display>& displays) {
  //MruWindowTracker::WindowList window_list =
      //Shell::Get()->mru_window_tracker()->BuildWindowForCycleList();
  //for (auto* window : window_list) {
  //}
}

void PersistentWindowController::OnDisplayModeChanged(
    const display::DisplayConfigurator::DisplayStateList& displays) {
  LOG(ERROR) << "on display mode changed...";
  if (GetDisplayManager()->IsInMirrorMode())
    LOG(ERROR) << "** is in mirror mode";
  else
    LOG(ERROR) << "** is not in mirror mode";
}

}  // namespace ash
