// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_DISPLAY_POWER_CONTROLLER_OBSERVER_H_
#define ASH_SYSTEM_POWER_DISPLAY_POWER_CONTROLLER_OBSERVER_H_

namespace ash {

class DisplayPowerControllerObserver {
 public:
  virtual ~DisplayPowerControllerObserver() = default;

  // Called when screen state tracked by DisplayPowerController changes.
  virtual void OnScreenStateChanged() {}

  // Called when DisplayPowerController starts or stops forcing backlights
  // off.
  virtual void OnBacklightsForcedOffChanged() {}

  // Called before display controller handles stylus eject, which might reset
  // requested display force off requests - this gives a chance to observers
  // that force display off on stylus removed event to do this preemptively,
  // before the controller attempts to change screen state.
  virtual void OnWillHandleStylusRemoved() {}
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_DISPLAY_POWER_CONTROLLER_OBSERVER_H_
