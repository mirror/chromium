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
    int64_t id,
    const gfx::Rect& display_bounds)
    : bounds_in_screen(bounds),
      display_id(id),
      display_bounds_in_screen(display_bounds) {}

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

  display::Screen* screen = display::Screen::GetScreen();
  for (auto* window : GetWindowList()) {
    wm::WindowState* window_state = wm::GetWindowState(window);
    const PersistentWindowInfo current_info(
        window->GetBoundsInScreen(),
        screen->GetDisplayNearestWindow(window).id(),
        screen->GetDisplayNearestWindow(window).bounds());
    // Save persistent window info when (1) it has not been saved, or (2) the
    // saved display id is valid but it is not equal to window's nearest display
    // id, which indicates a update is needed.
    if (!window_state->persistent_window_info()) {
      window_state->SetPersistentWindowInfo(current_info);
    } else {
      const auto& persistent_window_info =
          *window_state->persistent_window_info();
      const int64_t persistent_display_id = persistent_window_info.display_id;
      if (GetDisplayManager()->IsDisplayIdValid(persistent_display_id) &&
          persistent_display_id != current_info.display_id) {
        window_state->SetPersistentWindowInfo(current_info);
      }
    }
  }
}

void PersistentWindowController::OnDisplayConfigurationChanged() {
  if (!ShouldProcessWindowList())
    return;

  display::Screen* screen = display::Screen::GetScreen();
  for (auto* window : GetWindowList()) {
    wm::WindowState* window_state = wm::GetWindowState(window);
    if (!window_state->persistent_window_info())
      continue;
    const auto& persistent_window_info =
        *window_state->persistent_window_info();
    const int64_t persistent_display_id = persistent_window_info.display_id;
    if (persistent_display_id == screen->GetDisplayNearestWindow(window).id() ||
        !GetDisplayManager()->IsDisplayIdValid(persistent_display_id)) {
      continue;
    }

    // Update |persistent_window_bounds|'s position based on display's position
    // change. This ensures that |persistent_window_bounds| is associated with
    // the right target display.
    gfx::Rect persistent_window_bounds =
        persistent_window_info.bounds_in_screen;
    const auto& display =
        GetDisplayManager()->GetDisplayForId(persistent_display_id);
    const gfx::Vector2d offset =
        display.bounds().OffsetFromOrigin() -
        persistent_window_info.display_bounds_in_screen.OffsetFromOrigin();
    persistent_window_bounds.Offset(offset);

    window->SetBoundsInScreen(persistent_window_bounds, display);
  }
}

}  // namespace ash
