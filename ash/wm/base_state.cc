// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/base_state.h"
#include "ash/wm/wm_event.h"

namespace ash {
namespace wm {

BaseState::BaseState(mojom::WindowStateType initial_state_type)
    : state_type_(initial_state_type) {}
BaseState::~BaseState() = default;

void BaseState::OnWMEvent(WindowState* window_state, const WMEvent* event) {
  if (ProcessWorkspaceEvents(window_state, event))
    return;
  if (ProcessPinEvents(window_state, event))
    return;
  if (ProcessCompoundEvents(window_state, event))
    return;
  ProcessRegularEvents(window_state, event);
}

mojom::WindowStateType BaseState::GetType() const {
  return state_type_;
}

// static
mojom::WindowStateType BaseState::GetStateForEvent(const WMEvent* event) {
  switch (event->type()) {
    case WM_EVENT_NORMAL:
      return mojom::WindowStateType::NORMAL;
    case WM_EVENT_MAXIMIZE:
      return mojom::WindowStateType::MAXIMIZED;
    case WM_EVENT_MINIMIZE:
      return mojom::WindowStateType::MINIMIZED;
    case WM_EVENT_FULLSCREEN:
      return mojom::WindowStateType::FULLSCREEN;
    case WM_EVENT_SNAP_LEFT:
      return mojom::WindowStateType::LEFT_SNAPPED;
    case WM_EVENT_SNAP_RIGHT:
      return mojom::WindowStateType::RIGHT_SNAPPED;
    case WM_EVENT_SHOW_INACTIVE:
      return mojom::WindowStateType::INACTIVE;
    case WM_EVENT_PIN:
      return mojom::WindowStateType::PINNED;
    case WM_EVENT_TRUSTED_PIN:
      return mojom::WindowStateType::TRUSTED_PINNED;
    default:
      break;
  }
  if (event->IsWorkspaceEvent())
    NOTREACHED() << "Can't get the state for Workspace event";
  if (event->IsCompoundEvent())
    NOTREACHED() << "Can't get the state for Compound event";
  NOTREACHED() << "Can't get the state for Compound event";
  return mojom::WindowStateType::NORMAL;
}

}  // namespace wm
}  // namespace ash
