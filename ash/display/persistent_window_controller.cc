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
    if (window_state->persistent_window_info())
      continue;
    const auto& current_display = screen->GetDisplayNearestWindow(window);
    window_state->SetPersistentWindowInfo(
        PersistentWindowInfo(window->GetBoundsInScreen(), current_display.id(),
                             current_display.bounds()));
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
    if (persistent_display_id == screen->GetDisplayNearestWindow(window).id())
      continue;
    const auto& display =
        GetDisplayManager()->GetDisplayForId(persistent_display_id);
    if (!display.is_valid())
      continue;

    // Update |persistent_window_bounds| based on |persistent_display_bounds|'s
    // position change. This ensures that |persistent_window_bounds| is
    // associated with the right target display.
    gfx::Rect persistent_window_bounds =
        persistent_window_info.bounds_in_screen;
    const auto& persistent_display_bounds =
        persistent_window_info.display_bounds_in_screen;
    DCHECK(display.bounds().size() == persistent_display_bounds.size());
    const gfx::Vector2d offset = display.bounds().OffsetFromOrigin() -
                                 persistent_display_bounds.OffsetFromOrigin();
    persistent_window_bounds.Offset(offset);

    window->SetBoundsInScreen(persistent_window_bounds, display);
    // Reset persistent window info everytime the window bounds have restored.
    window_state->ResetPersistentWindowInfo();
  }
}

}  // namespace ash
