// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/persistent_window_controller.h"

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
  if (GetDisplayManager()->IsInMirrorMode())
    return;

  MruWindowTracker::WindowList window_list =
      Shell::Get()->mru_window_tracker()->BuildWindowForCycleList();
  for (auto* window : window_list) {
    const int64_t display_id = screen_->GetDisplayNearestWindow(window).id();
    wm::WindowState* window_state = wm::GetWindowState(window);
    if (window_state->persistent_window_bounds()) {
      const std::pair<gfx::Rect, int64_t> persistent_window_bounds =
          *window_state->persistent_window_bounds();
      const gfx::Rect persistent_bounds = persistent_window_bounds.first;
      const int64_t persistent_display_id = persistent_window_bounds.second;
      if (persistent_display_id != display_id ||
          persistent_bounds != window->GetBoundsInScreen()) {
        continue;
      }
    }
    LOG(ERROR) << "bounds to save: " << window->GetBoundsInScreen().ToString();
    LOG(ERROR) << "display id to save: " << display_id;
    window_state->SetPersistentWindowBounds(
        std::make_pair(window->GetBoundsInScreen(), display_id));
  }
}

void PersistentWindowController::OnDisplayConfigurationChanged() {
  LOG(ERROR) << "On display configuration changed...";
  if (GetDisplayManager()->IsInMirrorMode())
    return;
  LOG(ERROR) << "*** not mirror mode ***";

  MruWindowTracker::WindowList window_list =
      Shell::Get()->mru_window_tracker()->BuildWindowForCycleList();
  for (auto* window : window_list) {
    wm::WindowState* window_state = wm::GetWindowState(window);
    if (!window_state->persistent_window_bounds())
      continue;
    const std::pair<gfx::Rect, int64_t> persistent_window_bounds =
        *window_state->persistent_window_bounds();
    const gfx::Rect persistent_bounds = persistent_window_bounds.first;
    const int64_t persistent_display_id = persistent_window_bounds.second;
    const int64_t display_id = screen_->GetDisplayNearestWindow(window).id();
    LOG(ERROR) << "persistent_bounds: " << persistent_bounds.ToString();
    LOG(ERROR) << "persistent_display_id: " << persistent_display_id;
    LOG(ERROR) << "bounds here: " << window->GetBoundsInScreen().ToString();
    LOG(ERROR) << "display id here: " << display_id;
    if (persistent_display_id == display_id)
      continue;

    const display::Display target_display =
        display::Screen::GetScreen()->GetDisplayMatching(persistent_bounds);
    window->SetBoundsInScreen(persistent_bounds, target_display);
  }
}

}  // namespace ash
