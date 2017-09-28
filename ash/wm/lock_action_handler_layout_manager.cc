// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/lock_action_handler_layout_manager.h"

#include <utility>
#include <vector>

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/public/interfaces/tray_action.mojom.h"
#include "ash/shell.h"
#include "ash/system/lock_screen_action/lock_screen_action_background_controller.h"
#include "ash/tray_action/tray_action.h"
#include "ash/wm/lock_window_state.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "ash/wm/wm_event.h"
#include "base/bind.h"
#include "ui/wm/core/window_animations.h"

namespace ash {

namespace {

bool ShowChildWindows(mojom::TrayActionState action_state,
                      LockScreenActionBackgroundState background_state) {
  return action_state == mojom::TrayActionState::kActive &&
         (background_state == LockScreenActionBackgroundState::kShown ||
          background_state == LockScreenActionBackgroundState::kHidden);
}

}  // namespace

LockActionHandlerLayoutManager::LockActionHandlerLayoutManager(
    aura::Window* window,
    Shelf* shelf,
    std::unique_ptr<LockScreenActionBackgroundController>
        action_background_controller)
    : LockLayoutManager(window, shelf),
      action_background_controller_(std::move(action_background_controller)),
      tray_action_observer_(this),
      action_background_observer_(this) {
  TrayAction* tray_action = Shell::Get()->tray_action();
  tray_action_observer_.Add(tray_action);
  action_background_observer_.Add(action_background_controller_.get());
  action_background_controller_->SetParentWindow(window);
}

LockActionHandlerLayoutManager::~LockActionHandlerLayoutManager() = default;

void LockActionHandlerLayoutManager::OnWindowAddedToLayout(
    aura::Window* child) {
  ::wm::SetWindowVisibilityAnimationTransition(child, ::wm::ANIMATE_NONE);

  wm::WindowState* window_state =
      action_background_controller_->IsBackgroundWindow(child)
          ? LockWindowState::SetLockWindowState(child)
          : LockWindowState::SetLockWindowStateWithShelfExcluded(child);
  wm::WMEvent event(wm::WM_EVENT_ADDED_TO_WORKSPACE);
  window_state->OnWMEvent(&event);
}

void LockActionHandlerLayoutManager::OnChildWindowVisibilityChanged(
    aura::Window* child,
    bool visible) {
  if (action_background_controller_->IsBackgroundWindow(child)) {
    window()->StackChildAtBottom(child);
    return;
  }

  // Windows should be shown only in active state.
  if (visible &&
      !ShowChildWindows(Shell::Get()->tray_action()->GetLockScreenNoteState(),
                        action_background_controller_->state())) {
    child->Hide();
  }
}

void LockActionHandlerLayoutManager::OnLockScreenNoteStateChanged(
    mojom::TrayActionState state) {
  if (state == mojom::TrayActionState::kAvailable) {
    if (action_background_controller_->HideBackground())
      return;
  }

  if (state == mojom::TrayActionState::kNotAvailable) {
    if (action_background_controller_->HideBackgroundImmediately())
      return;
  }

  if (state == mojom::TrayActionState::kLaunching ||
      state == mojom::TrayActionState::kActive) {
    if (action_background_controller_->ShowBackground())
      return;
  }

  UpdateChildren(state, action_background_controller_->state());
}

void LockActionHandlerLayoutManager::OnLockScreenActionBackgroundStateChanged(
    LockScreenActionBackgroundState state) {
  UpdateChildren(Shell::Get()->tray_action()->GetLockScreenNoteState(), state);
}

void LockActionHandlerLayoutManager::UpdateChildren(
    mojom::TrayActionState action_state,
    LockScreenActionBackgroundState background_state) {
  // Update children state:
  // * a child can be visible only in active state
  // * on transition to active state:
  //     * show hidden windows, so children that were added when action was not
  //       in active state are shown
  //     * activate a container child to ensure the container gets focus when
  //       moving from background state.
  bool show_children = ShowChildWindows(action_state, background_state);
  aura::Window* child_to_activate = nullptr;
  for (aura::Window* child : window()->children()) {
    if (action_background_controller_->IsBackgroundWindow(child))
      continue;
    if (show_children) {
      child->Show();
      child_to_activate = child;
    } else {
      child->Hide();
    }
  }

  if (child_to_activate)
    wm::ActivateWindow(child_to_activate);
}

}  // namespace ash
