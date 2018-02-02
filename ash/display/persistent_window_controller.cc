// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/display/persistent_window_controller.h"

#include "ash/public/cpp/ash_switches.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/wm/mru_window_tracker.h"
#include "ash/wm/window_state.h"
#include "base/command_line.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/screen.h"

namespace ash {

namespace {

display::DisplayManager* GetDisplayManager() {
  return Shell::Get()->display_manager();
}

MruWindowTracker::WindowList GetWindowList() {
  return Shell::Get()->mru_window_tracker()->BuildWindowForCycleList();
}

// Returns true when window cycle list can be processed to perform save/restore
// operations on observing display changes.
bool ShouldProcessWindowList() {
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAshEnablePersistentWindowBounds)) {
    return false;
  }

  // Window cycle list exists in active user session only.
  if (Shell::Get()->session_controller()->IsUserSessionBlocked())
    return false;

  // TODO(warx): Decide on the correct behavior involving mixed mirror mode.
  if (GetDisplayManager()->IsInMirrorMode())
    return false;

  return true;
}

}  // namespace

PersistentWindowController::PersistentWindowInfo::PersistentWindowInfo(
    const gfx::Rect& bounds,
    int64_t id)
    : bounds_in_screen(bounds), display_id(id) {}

PersistentWindowController::PersistentWindowInfo::~PersistentWindowInfo() =
    default;

PersistentWindowController::PersistentWindowController() {
  display::Screen::GetScreen()->AddObserver(this);
  Shell::Get()->window_tree_host_manager()->AddObserver(this);
}

PersistentWindowController::~PersistentWindowController() {
  Shell::Get()->window_tree_host_manager()->RemoveObserver(this);
  display::Screen::GetScreen()->RemoveObserver(this);
}

void PersistentWindowController::OnWillProcessDisplayChanges() {
  if (!ShouldProcessWindowList())
    return;

  for (auto* window : GetWindowList()) {
    wm::WindowState* window_state = wm::GetWindowState(window);
    // Do not save persistent window info if it has been saved, as display
    // changes like display connected could have made window positioned on new
    // display.
    // TODO(warx): This relies on we reset persistent window info properly,
    // which is fragile.
    if (window_state->persistent_window_info())
      continue;
    const int64_t& display_id =
        display::Screen::GetScreen()->GetDisplayNearestWindow(window).id();
    window_state->SetPersistentWindowInfo(
        PersistentWindowInfo(window->GetBoundsInScreen(), display_id));
  }
}

void PersistentWindowController::OnDisplayConfigurationChanged() {
  if (!ShouldProcessWindowList())
    return;

  for (auto* window : GetWindowList()) {
    wm::WindowState* window_state = wm::GetWindowState(window);
    if (!window_state->persistent_window_info())
      continue;
    const auto& persistent_window_info =
        *window_state->persistent_window_info();
    const int64_t& persistent_display_id = persistent_window_info.display_id;
    display::Screen* screen = display::Screen::GetScreen();
    if (persistent_display_id == screen->GetDisplayNearestWindow(window).id() ||
        !GetDisplayManager()->IsDisplayIdValid(persistent_display_id)) {
      continue;
    }

    const gfx::Rect& persistent_window_bounds =
        persistent_window_info.bounds_in_screen;
    const auto& display =
        GetDisplayManager()->GetDisplayForId(persistent_display_id);
    window->SetBoundsInScreen(persistent_window_bounds, display);
  }
}

}  // namespace ash
