// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/client_controlled_state.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/public/cpp/window_state_type.h"
#include "ash/root_window_controller.h"
#include "ash/screen_util.h"
#include "ash/shell.h"
#include "ash/wm/screen_pinning_controller.h"
#include "ash/wm/window_animation_types.h"
#include "ash/wm/window_parenting_utils.h"
#include "ash/wm/window_positioning_utils.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_state_delegate.h"
#include "ash/wm/window_state_util.h"
#include "ash/wm/wm_event.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/wm/core/window_util.h"

namespace ash {
namespace wm {
namespace {

bool IsMinimizedWindowState(const mojom::WindowStateType state_type) {
  return state_type == mojom::WindowStateType::MINIMIZED;
}

}  // namespace

ClientControlledState::ClientControlledState(std::unique_ptr<Delegate> delegate)
    : BaseState(mojom::WindowStateType::DEFAULT),
      delegate_(std::move(delegate)) {}

ClientControlledState::~ClientControlledState() {}

void ClientControlledState::ProcessRegularEvents(WindowState* window_state,
                                                 const WMEvent* event) {
  mojom::WindowStateType current_state_type = window_state->GetStateType();
  LOG(ERROR) << "Processing Regular Event: event=" << event->type()
             << ", current state=" << current_state_type;

  switch (event->type()) {
    case WM_EVENT_NORMAL:
    case WM_EVENT_MAXIMIZE:
    case WM_EVENT_MINIMIZE:
    case WM_EVENT_FULLSCREEN:
    case WM_EVENT_SNAP_LEFT:
    case WM_EVENT_SNAP_RIGHT:
      delegate_->SendWindowStateRequest(current_state_type,
                                        GetStateForEvent(event));
      // Reset window state
      window_state->UpdateWindowPropertiesFromStateType();
      break;
    case WM_EVENT_SET_BOUNDS: {
      // TODO(oshima): Send the set bounds request to client.
      const SetBoundsEvent* set_bounds_event =
          static_cast<const SetBoundsEvent*>(event);
      if (set_bounds_locally_) {
        window_state->SetBoundsDirect(set_bounds_event->requested_bounds());
      } else {
        delegate_->SendBoundsRequest(current_state_type,
                                     set_bounds_event->requested_bounds());
      }
      break;
    }
    case WM_EVENT_SHOW_INACTIVE:
      NOTREACHED();
      // next_state_type = mojom::WindowStateType::INACTIVE;
      break;
    case WM_EVENT_PIN:
    case WM_EVENT_TRUSTED_PIN:
      NOTREACHED() << "Pinned event should not reach here:" << event;
      return;
    case WM_EVENT_TOGGLE_MAXIMIZE_CAPTION:
    case WM_EVENT_TOGGLE_MAXIMIZE:
    case WM_EVENT_TOGGLE_VERTICAL_MAXIMIZE:
    case WM_EVENT_TOGGLE_HORIZONTAL_MAXIMIZE:
    case WM_EVENT_TOGGLE_FULLSCREEN:
    case WM_EVENT_CYCLE_SNAP_LEFT:
    case WM_EVENT_CYCLE_SNAP_RIGHT:
    case WM_EVENT_CENTER:
      NOTREACHED() << "Compound event should not reach here:" << event;
      return;
    case WM_EVENT_ADDED_TO_WORKSPACE:
    case WM_EVENT_WORKAREA_BOUNDS_CHANGED:
    case WM_EVENT_DISPLAY_BOUNDS_CHANGED:
      NOTREACHED() << "Workspace event should not reach here:" << event;
      return;
  }
}

void ClientControlledState::AttachState(
    WindowState* window_state,
    WindowState::State* state_in_previous_mode) {}

void ClientControlledState::DetachState(WindowState* window_state) {}

bool ClientControlledState::ProcessPinEvents(WindowState* window_state,
                                             const WMEvent* event) {
  // Do not change the PINNED window state if this is not unpin event.
  if (window_state->IsTrustedPinned()) {
    if (event->type() == WM_EVENT_NORMAL) {
      // Unpin
      delegate_->SendWindowStateRequest(state_type_,
                                        mojom::WindowStateType::NORMAL);
      EnterToNextState(window_state, mojom::WindowStateType::NORMAL);
    }
    return true;
  }
  if ((window_state->IsPinned() && event->IsStateTransitionEvent()) ||
      event->IsPinEvent()) {
    if (window_state->IsPinned()) {
      if (event->type() == WM_EVENT_FULLSCREEN) {
        LOG(ERROR) << "Skipping Fullscreen request in pinned state";
        return true;
      }
    }
    mojom::WindowStateType next_state_type = GetStateForEvent(event);
    // Unpin
    delegate_->SendWindowStateRequest(state_type_, next_state_type);
    bool pinned = window_state->IsPinned();
    EnterToNextState(window_state, next_state_type);
    LOG(ERROR) << "Processing Pinned Event: event=" << event->type()
               << ", window=" << pinned << "=>" << window_state->IsPinned()
               << ", next=" << next_state_type;
    return true;
  }
  return false;
}

bool ClientControlledState::ProcessCompoundEvents(WindowState* window_state,
                                                  const WMEvent* event) {
  switch (event->type()) {
    case WM_EVENT_TOGGLE_MAXIMIZE_CAPTION:
      if (window_state->IsFullscreen()) {
        const wm::WMEvent event(wm::WM_EVENT_TOGGLE_FULLSCREEN);
        window_state->OnWMEvent(&event);
      } else if (window_state->IsMaximized()) {
        window_state->Restore();
      } else if (window_state->IsNormalOrSnapped()) {
        if (window_state->CanMaximize())
          window_state->Maximize();
      }
      return true;
    case WM_EVENT_TOGGLE_MAXIMIZE:
      if (window_state->IsFullscreen()) {
        const wm::WMEvent event(wm::WM_EVENT_TOGGLE_FULLSCREEN);
        window_state->OnWMEvent(&event);
      } else if (window_state->IsMaximized()) {
        window_state->Restore();
      } else if (window_state->CanMaximize()) {
        window_state->Maximize();
      }
      return true;
    case WM_EVENT_TOGGLE_VERTICAL_MAXIMIZE: {
      // TODO(oshima): Implement this.
      return false;
    }
    case WM_EVENT_TOGGLE_HORIZONTAL_MAXIMIZE: {
      // TODO(oshima): Implement this.
      return false;
    }
    case WM_EVENT_TOGGLE_FULLSCREEN:
      ToggleFullScreen(window_state, window_state->delegate());
      return true;
    case WM_EVENT_CYCLE_SNAP_LEFT:
    case WM_EVENT_CYCLE_SNAP_RIGHT:
      // TODO(oshima): implement this.
      return true;
    case WM_EVENT_CENTER:
      // TODO(oshima): implement this.
      return true;
    case WM_EVENT_NORMAL:
    case WM_EVENT_MAXIMIZE:
    case WM_EVENT_MINIMIZE:
    case WM_EVENT_FULLSCREEN:
    case WM_EVENT_PIN:
    case WM_EVENT_TRUSTED_PIN:
    case WM_EVENT_SNAP_LEFT:
    case WM_EVENT_SNAP_RIGHT:
    case WM_EVENT_SET_BOUNDS:
    case WM_EVENT_SHOW_INACTIVE:
      break;
    case WM_EVENT_ADDED_TO_WORKSPACE:
    case WM_EVENT_WORKAREA_BOUNDS_CHANGED:
    case WM_EVENT_DISPLAY_BOUNDS_CHANGED:
      NOTREACHED() << "Workspace event should not reach here:" << event;
      break;
  }
  return false;
}

bool ClientControlledState::ProcessWorkspaceEvents(WindowState* window_state,
                                                   const WMEvent* event) {
  switch (event->type()) {
    case WM_EVENT_ADDED_TO_WORKSPACE:
    case WM_EVENT_DISPLAY_BOUNDS_CHANGED:
    case WM_EVENT_WORKAREA_BOUNDS_CHANGED: {
      // Client is responsible for adjusting bounds after display bounds change.
      return true;
    }
    case WM_EVENT_TOGGLE_MAXIMIZE_CAPTION:
    case WM_EVENT_TOGGLE_MAXIMIZE:
    case WM_EVENT_TOGGLE_VERTICAL_MAXIMIZE:
    case WM_EVENT_TOGGLE_HORIZONTAL_MAXIMIZE:
    case WM_EVENT_TOGGLE_FULLSCREEN:
    case WM_EVENT_CYCLE_SNAP_LEFT:
    case WM_EVENT_CYCLE_SNAP_RIGHT:
    case WM_EVENT_CENTER:
    case WM_EVENT_NORMAL:
    case WM_EVENT_MAXIMIZE:
    case WM_EVENT_MINIMIZE:
    case WM_EVENT_FULLSCREEN:
    case WM_EVENT_PIN:
    case WM_EVENT_TRUSTED_PIN:
    case WM_EVENT_SNAP_LEFT:
    case WM_EVENT_SNAP_RIGHT:
    case WM_EVENT_SET_BOUNDS:
    case WM_EVENT_SHOW_INACTIVE:
      break;
  }
  return false;
}

bool ClientControlledState::EnterToNextState(
    WindowState* window_state,
    mojom::WindowStateType next_state_type) {
  // Do nothing if  we're already in the same state.
  if (state_type_ == next_state_type)
    return false;

  mojom::WindowStateType previous_state_type = state_type_;
  state_type_ = next_state_type;

  window_state->UpdateWindowPropertiesFromStateType();
  window_state->NotifyPreStateTypeChange(previous_state_type);

  if (window_state->window()->parent()) {
    UpdateBoundsFromState(window_state, previous_state_type);
  }

  window_state->NotifyPostStateTypeChange(previous_state_type);

  if (next_state_type == mojom::WindowStateType::PINNED ||
      previous_state_type == mojom::WindowStateType::PINNED ||
      next_state_type == mojom::WindowStateType::TRUSTED_PINNED ||
      previous_state_type == mojom::WindowStateType::TRUSTED_PINNED) {
    Shell::Get()->screen_pinning_controller()->SetPinnedWindow(
        window_state->window());
  }
  return true;
}

void ClientControlledState::UpdateBoundsFromState(
    WindowState* window_state,
    mojom::WindowStateType previous_state_type) {
  aura::Window* window = window_state->window();
  gfx::Rect bounds_in_parent;
  switch (state_type_) {
    case mojom::WindowStateType::LEFT_SNAPPED:
    case mojom::WindowStateType::RIGHT_SNAPPED:
      bounds_in_parent =
          state_type_ == mojom::WindowStateType::LEFT_SNAPPED
              ? GetDefaultLeftSnappedWindowBoundsInParent(window)
              : GetDefaultRightSnappedWindowBoundsInParent(window);
      break;

    case mojom::WindowStateType::DEFAULT:
    case mojom::WindowStateType::NORMAL: {
      break;
    }
    case mojom::WindowStateType::MAXIMIZED:
      bounds_in_parent = ScreenUtil::GetMaximizedWindowBoundsInParent(window);
      break;

    case mojom::WindowStateType::FULLSCREEN:
    case mojom::WindowStateType::PINNED:
    case mojom::WindowStateType::TRUSTED_PINNED:
      bounds_in_parent = ScreenUtil::GetDisplayBoundsInParent(window);
      break;

    case mojom::WindowStateType::MINIMIZED:
      break;
    case mojom::WindowStateType::INACTIVE:
    case mojom::WindowStateType::AUTO_POSITIONED:
      return;
  }

  if (!window_state->IsMinimized()) {
    if (IsMinimizedWindowState(previous_state_type) ||
        window_state->IsFullscreen() || window_state->IsPinned()) {
      window_state->SetBoundsDirect(bounds_in_parent);
    } else if (window_state->IsMaximized() ||
               IsMaximizedOrFullscreenOrPinnedWindowStateType(
                   previous_state_type)) {
      window_state->SetBoundsDirectCrossFade(bounds_in_parent);
    } else if (window_state->is_dragged()) {
// SetBoundsDirectAnimated does not work when the window gets reparented.
// TODO(oshima): Consider fixing it and reenable the animation.
#if 0
      window_state->SetBoundsDirect(bounds_in_parent);
#endif
    } else {
      window_state->SetBoundsDirectAnimated(bounds_in_parent);
    }
  }

  if (window_state->IsMinimized()) {
    // Save the previous show state so that we can correctly restore it after
    // exiting the minimized mode.
    window->SetProperty(aura::client::kPreMinimizedShowStateKey,
                        ToWindowShowState(previous_state_type));
    ::wm::SetWindowVisibilityAnimationType(
        window, WINDOW_VISIBILITY_ANIMATION_TYPE_MINIMIZE);

    window->Hide();
    if (window_state->IsActive())
      window_state->Deactivate();
  } else if ((window->layer()->GetTargetVisibility() ||
              IsMinimizedWindowState(previous_state_type)) &&
             !window->layer()->visible()) {
    // The layer may be hidden if the window was previously minimized. Make
    // sure it's visible.
    window->Show();
    if (IsMinimizedWindowState(previous_state_type) &&
        !window_state->IsMaximizedOrFullscreenOrPinned()) {
      window_state->set_unminimize_to_restore_bounds(false);
    }
  }
}

}  // namespace wm
}  // namespace ash
