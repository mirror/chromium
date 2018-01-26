// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/persistent_window_controller.h"

#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/wm/mru_window_tracker.h"
#include "ash/wm/window_state.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/screen.h"

namespace ash {

namespace {

display::DisplayManager* GetDisplayManager() {
  return Shell::Get()->display_manager();
}

}  // namespace

PersistentWindowController::PersistentWindowController(
    WindowTreeHostManager* window_tree_host_manager)
    : screen_(display::Screen::GetScreen()),
      window_tree_host_manager_(window_tree_host_manager) {
  screen_->AddObserver(this);
  window_tree_host_manager_->AddObserver(this);
}

PersistentWindowController::~PersistentWindowController() {
  window_tree_host_manager_->RemoveObserver(this);
  screen_->RemoveObserver(this);
}

void PersistentWindowController::OnWillProcessDisplayChanges() {
  if (Shell::Get()->session_controller()->IsUserSessionBlocked())
    return;

  if (GetDisplayManager()->IsInMirrorMode())
    return;

  MruWindowTracker::WindowList window_list =
      Shell::Get()->mru_window_tracker()->BuildWindowForCycleList();
  for (auto* window : window_list) {
    wm::WindowState* window_state = wm::GetWindowState(window);
    if (!window_state->persistent_window_info()) {
      const int64_t display_id = screen_->GetDisplayNearestWindow(window).id();
      window_state->SetPersistentWindowInfo(
          std::make_pair(display_id, window->GetBoundsInScreen()));
    }
  }
}

void PersistentWindowController::OnDisplayConfigurationChanged() {
  if (Shell::Get()->session_controller()->IsUserSessionBlocked())
    return;
  if (GetDisplayManager()->IsInMirrorMode())
    return;

  MruWindowTracker::WindowList window_list =
      Shell::Get()->mru_window_tracker()->BuildWindowForCycleList();
  for (auto* window : window_list) {
    wm::WindowState* window_state = wm::GetWindowState(window);
    if (!window_state->persistent_window_info())
      continue;
    const auto persistent_window_info = *window_state->persistent_window_info();
    const int64_t persistent_display_id = persistent_window_info.first;
    const gfx::Rect persistent_window_bounds = persistent_window_info.second;
    if (persistent_display_id ==
            screen_->GetDisplayNearestWindow(window).id() ||
        !GetDisplayManager()->IsDisplayIdValid(persistent_display_id)) {
      continue;
    }

    const auto display = display::Screen::GetScreen()->GetDisplayMatching(
        persistent_window_bounds);
    window->SetBoundsInScreen(persistent_window_bounds, display);
    window_state->ResetPersistentWindowInfo();
  }
}

}  // namespace ash
