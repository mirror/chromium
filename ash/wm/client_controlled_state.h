// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_CLIENT_CONTROLLED_STATE_H_
#define ASH_WM_CLIENT_CONTROLLED_STATE_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/wm/base_state.h"
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
class ASH_EXPORT ClientControlledState : public BaseState {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual void SendWindowStateRequest(mojom::WindowStateType current_state,
                                        mojom::WindowStateType next_state);
    virtual void SendBoundsRequest(mojom::WindowStateType current_state,
                                   const gfx::Rect& bounds) = 0;
  };

  ClientControlledState(std::unique_ptr<Delegate> delegate);
  ~ClientControlledState() override;

  // WindowState::State overrides:
  void AttachState(WindowState* window_state,
                   WindowState::State* previous_state) override;
  void DetachState(WindowState* window_state) override;

  void set_bounds_locally(bool set) { set_bounds_locally_ = set; }

  // BaseState:
  bool ProcessCompoundEvents(WindowState* window_state,
                             const WMEvent* event) override;
  bool ProcessWorkspaceEvents(WindowState* window_state,
                              const WMEvent* event) override;
  bool ProcessPinEvents(WindowState* window_state,
                        const WMEvent* event) override;
  void ProcessRegularEvents(WindowState* window_state,
                            const WMEvent* event) override;

  // Enters next state. This is used when the state moves from one to another
  // within the same desktop mode.
  bool EnterToNextState(wm::WindowState* window_state,
                        mojom::WindowStateType next_state_type);

 private:
  // Animates to new window bounds based on the current and previous state type.
  void UpdateBoundsFromState(wm::WindowState* window_state,
                             mojom::WindowStateType old_state_type);

  std::unique_ptr<Delegate> delegate_;

  bool set_bounds_locally_ = false;

  DISALLOW_COPY_AND_ASSIGN(ClientControlledState);
};

}  // namespace wm
}  // namespace ash

#endif  // ASH_WM_DEFAULT_STATE_H_
