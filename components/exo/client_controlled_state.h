// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_CLIENT_CONTROLLED_STATE_H_
#define COMPONENTS_EXO_CLIENT_CONTROLLED_STATE_H_

#include "ash/wm/window_state.h"
#include "base/macros.h"
#include "ui/display/display.h"
#include "ui/gfx/geometry/rect.h"

namespace ash {
namespace mojom {
enum class WindowStateType;
}

namespace wm {
class SetBoundsEvent;

// ClientControlledState implements a behavior of the window whose
// window state is determined by the delegate.
class ClientControlledState : public WindowState::State {
 public:
  class ClientControlStateDelegate {
   public:
    void SendWindowStateRequest(mojom::WindowStateType type);
    void SendSetBoundsRequest(const gfx::Rect& bounds);
  };

  explicit ClientControlledState(mojom::WindowStateType initial_state_type);
  ~ClientControlledState() override;

  // WindowState::State overrides:
  void OnWMEvent(WindowState* window_state, const WMEvent* event) override;
  mojom::WindowStateType GetType() const override;
  void AttachState(WindowState* window_state,
                   WindowState::State* previous_state) override;
  void DetachState(WindowState* window_state) override;

 private:
  // Process state dependent events, such as TOGGLE_MAXIMIZED,
  // TOGGLE_FULLSCREEN.
  static bool ProcessCompoundEvents(WindowState* window_state,
                                    const WMEvent* event);

  // Process workspace related events, such as DISPLAY_BOUNDS_CHANGED.
  static bool ProcessWorkspaceEvents(WindowState* window_state,
                                     const WMEvent* event);

  // Set the fullscreen/maximized bounds without animation.
  static bool SetMaximizedOrFullscreenBounds(wm::WindowState* window_state);

  static void SetBounds(WindowState* window_state,
                        const SetBoundsEvent* bounds_event);

  static void CenterWindow(WindowState* window_state);

  // Enters next state. This is used when the state moves from one to another
  // within the same desktop mode.
  void EnterToNextState(wm::WindowState* window_state,
                        mojom::WindowStateType next_state_type);

  // Reenters the current state. This is called when migrating from
  // previous desktop mode, and the window's state needs to re-construct the
  // state/bounds for this state.
  void ReenterToCurrentState(wm::WindowState* window_state,
                             wm::WindowState::State* state_in_previous_mode);

  // Animates to new window bounds based on the current and previous state type.
  void UpdateBoundsFromState(wm::WindowState* window_state,
                             mojom::WindowStateType old_state_type);

  void RequestboundsChange(const gfx::Rect& bounds);

  // The current type of the window.
  mojom::WindowStateType state_type_;

  // The saved window state for the case that the state gets de-/activated.
  gfx::Rect stored_bounds_;
  gfx::Rect stored_restore_bounds_;

  // The display state in which the mode got started.
  display::Display stored_display_state_;

  ClientControlStateDelegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(ClientControlledState);
};

}  // namespace wm
}  // namespace ash

#endif  // ASH_WM_DEFAULT_STATE_H_
