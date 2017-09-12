// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_INPUTFD_EVDEV_SEAT_H_
#define COMPONENTS_EXO_INPUTFD_EVDEV_SEAT_H_

#include <memory>
#include "base/containers/flat_map.h"
#include "base/macros.h"
#include "components/exo/wm_helper.h"
#include "ui/aura/client/focus_change_observer.h"
#include "ui/events/ozone/gamepad/gamepad_observer.h"

namespace exo {
class InputfdEvdevSeatDelegate;
class InputfdEvdevDevice;
class Surface;

class InputfdEvdevSeat : public WMHelper::FocusObserver,
                         public ui::GamepadObserver {
 public:
  InputfdEvdevSeat(InputfdEvdevSeatDelegate* delegate);

  ~InputfdEvdevSeat() override;

  // Overridden from WMHelper::FocusObserver:
  void OnWindowFocused(aura::Window* gained_focus,
                       aura::Window* lost_focus) override;

  // Overridden from ui::GamepadObserver:
  void OnGamepadDevicesUpdated() override;

 private:
  // The delegate that handles gamepad_added.
  InputfdEvdevSeatDelegate* const delegate_;

  // Contains the delegate for each gamepads_ device.
  base::flat_map<int, std::unique_ptr<InputfdEvdevDevice>> gamepads_;

  // Disptach focus in to every device.
  void DispatchFocusIn(Surface* surface);

  // Dispatch focus out to every device.
  void DispatchFocusOut();

  // The flag if a valid target for gaming seat is focused. In other words, if
  // it's true, this class is observing gamepad events.
  bool focused_ = false;

  DISALLOW_COPY_AND_ASSIGN(InputfdEvdevSeat);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_INPUTFD_EVDEV_SEAT_H_
