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

bool IsWindowInDisplayList(aura::Window* window,
                           const std::vector<display::Display>& displays) {
  const display::Display display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(window);
  LOG(ERROR) << "display id here: " << display.id();
  for (auto& d : displays) {
    if (d.id() == display.id())
      return true;
  }
  return false;
}

}  // namespace

PersistentWindowController::PersistentWindowController(
    WindowTreeHostManager* window_tree_host_manager)
    : screen_(display::Screen::GetScreen()),
      window_tree_host_manager_(window_tree_host_manager) {
  Shell::Get()->display_configurator()->AddObserver(this);
  screen_->AddObserver(this);
  window_tree_host_manager_->AddObserver(this);
}

PersistentWindowController::~PersistentWindowController() {
  window_tree_host_manager_->RemoveObserver(this);
  Shell::Get()->display_configurator()->RemoveObserver(this);
  screen_->RemoveObserver(this);
}

void PersistentWindowController::SavePersistentWindowBounds(
    const std::vector<display::Display>& displays) {
  MruWindowTracker::WindowList window_list =
      Shell::Get()->mru_window_tracker()->BuildWindowForCycleList();
  for (auto* window : window_list) {
    if (!IsWindowInDisplayList(window, displays))
      continue;
    wm::WindowState* window_state = wm::GetWindowState(window);
    LOG(ERROR) << "bounds to save: " << window->GetBoundsInScreen().ToString();
    window_state->SetPreDisplayWindowBounds(window->GetBoundsInScreen());
  }
  configured_ = false;
}

void PersistentWindowController::OnDisplayModeChanged(
    const display::DisplayConfigurator::DisplayStateList& displays) {
}

void PersistentWindowController::OnDisplayRemoved(
    const display::Display& old_display) {
  LOG(ERROR) << "on display removed.......";
  if (GetDisplayManager()->IsInMirrorMode())
    return;
  SavePersistentWindowBounds({old_display});
}

void PersistentWindowController::OnDisplayConfigurationChanged() {
  LOG(ERROR) << "on display configuration changed...";
  if (!configured_) {
    LOG(ERROR) << "!configured_";
    configured_ = true;
    return;
  }

  MruWindowTracker::WindowList window_list =
      Shell::Get()->mru_window_tracker()->BuildWindowForCycleList();
  LOG(ERROR) << "window list size: " << window_list.size();
  for (auto* window : window_list) {
    wm::WindowState* window_state = wm::GetWindowState(window);
    if (!window_state->pre_display_window_bounds()) {
      LOG(ERROR) << "i dont have pre display window bounds, my bounds is: " << window->GetBoundsInScreen().ToString();
      continue;
    }
    const gfx::Rect pre_display_window_bounds =
        *window_state->pre_display_window_bounds();
    LOG(ERROR) << "pre display window bounds: " << pre_display_window_bounds.ToString();
    if (window->GetBoundsInScreen() == pre_display_window_bounds)
      continue;
    const display::Display target_display =
        display::Screen::GetScreen()->GetDisplayMatching(
            pre_display_window_bounds);
    window->SetBoundsInScreen(pre_display_window_bounds, target_display);
  }
  configured_ = false;
}

}  // namespace ash
