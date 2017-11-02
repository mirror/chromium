// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/window_state.h"

#include "base/macros.h"

namespace ash {
namespace wm {

class BaseState : public WindowState::State {
 public:
  BaseState(mojom::WindowStateType initial_state_type);
  ~BaseState() override;

  void OnWMEvent(WindowState* window_state, const WMEvent* event) override;
  mojom::WindowStateType GetType() const override;

 protected:
  static mojom::WindowStateType GetStateForEvent(const WMEvent* event);

  // Process state dependent events, such as TOGGLE_MAXIMIZED,
  // TOGGLE_FULLSCREEN.
  virtual bool ProcessCompoundEvents(WindowState* window_state,
                                     const WMEvent* event) = 0;

  // Process workspace related events, such as DISPLAY_BOUNDS_CHANGED.
  virtual bool ProcessWorkspaceEvents(WindowState* window_state,
                                      const WMEvent* event) = 0;

  // Process pin related events, such as PIN_TRUSTED
  virtual bool ProcessPinEvents(WindowState* window_state,
                                const WMEvent* event) = 0;

  virtual void ProcessRegularEvents(WindowState* window_state,
                                    const WMEvent* event) = 0;

  // The current type of the window.
  mojom::WindowStateType state_type_;

 private:
  DISALLOW_COPY_AND_ASSIGN(BaseState);
};

}  // namespace wm
}  // namespace ash
