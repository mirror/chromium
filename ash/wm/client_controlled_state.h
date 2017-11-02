// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_CLIENT_CONTROLLED_STATE_H_
#define ASH_WM_CLIENT_CONTROLLED_STATE_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/wm/window_state.h"
#include "ash/wm/wm_event.h"
#include "base/macros.h"
#include "ui/display/display.h"
#include "ui/gfx/geometry/rect.h"

namespace ash {
namespace mojom {
enum class WindowStateType;
}

namespace wm {

// ClientControlledState implements a behavior of the window whose
// window state is determined by the delegate.
class ASH_EXPORT ClientControlledState : public WindowState::State {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual void SendWindowStateRequest(mojom::WindowStateType current_state,
                                        WMEventType type) = 0;
    virtual void SendBoundsRequest(mojom::WindowStateType current_state,
                                   const gfx::Rect& bounds) = 0;
  };

  ClientControlledState(std::unique_ptr<Delegate> delegate);
  ~ClientControlledState() override;

  // WindowState::State overrides:
  void OnWMEvent(WindowState* window_state, const WMEvent* event) override;
  mojom::WindowStateType GetType() const override;
  void AttachState(WindowState* window_state,
                   WindowState::State* previous_state) override;
  void DetachState(WindowState* window_state) override;

  void SetBounds(WindowState* window_state, const gfx::Rect& bounds);

 private:
  // Process state dependent events, such as TOGGLE_MAXIMIZED,
  // TOGGLE_FULLSCREEN.
  bool ProcessCompoundEvents(WindowState* window_state, const WMEvent* event);

  // Process workspace related events, such as DISPLAY_BOUNDS_CHANGED.
  static bool ProcessWorkspaceEvents(WindowState* window_state,
                                     const WMEvent* event);

 public:
  // Enters next state. This is used when the state moves from one to another
  // within the same desktop mode.
  void EnterToNextState(wm::WindowState* window_state,
                        mojom::WindowStateType next_state_type);

 private:
  // Animates to new window bounds based on the current and previous state type.
  void UpdateBoundsFromState(wm::WindowState* window_state,
                             mojom::WindowStateType old_state_type);

  // The current type of the window.
  mojom::WindowStateType state_type_ = mojom::WindowStateType::DEFAULT;

  std::unique_ptr<Delegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(ClientControlledState);
};

}  // namespace wm
}  // namespace ash

#endif  // ASH_WM_DEFAULT_STATE_H_
