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

  // TODO(warx): Reconcile after properly defining the behavior involving mixed
  // mode mirroring.
  if (GetDisplayManager()->IsInMirrorMode())
    return false;

  return true;
}

}  // namespace

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
    // TODO(warx): Reset persistent_window_info properly or save only when (1)
    // there is no saved persistent window info, (2) the saved display id is
    // valid but is not equal to the id of display with largest intersection.
    if (window_state->persistent_window_info())
      continue;
    PersistentWindowInfo info;
    info.bounds_in_screen = window->GetBoundsInScreen();
    info.display_id =
        display::Screen::GetScreen()->GetDisplayNearestWindow(window).id();
    window_state->SetPersistentWindowInfo(info);
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
    const int64_t persistent_display_id = persistent_window_info.display_id;
    if (persistent_display_id == display::Screen::GetScreen()
                                     ->GetDisplayNearestWindow(window)
                                     .id() ||
        !GetDisplayManager()->IsDisplayIdValid(persistent_display_id)) {
      continue;
    }

    const gfx::Rect& persistent_window_bounds =
        persistent_window_info.bounds_in_screen;
    const auto display = display::Screen::GetScreen()->GetDisplayMatching(
        persistent_window_bounds);
    window->SetBoundsInScreen(persistent_window_bounds, display);
  }
}

}  // namespace ash
