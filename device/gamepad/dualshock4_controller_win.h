// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_DUALSHOCK4_CONTROLLER_WIN_
#define DEVICE_GAMEPAD_DUALSHOCK4_CONTROLLER_WIN_

#include <Unknwn.h>
#include <WinDef.h>
#include <stdint.h>
#include <windows.h>

#include "base/win/scoped_handle.h"
#include "device/gamepad/public/cpp/gamepad.h"
#include "device/gamepad/public/interfaces/gamepad.mojom.h"

namespace device {

class Dualshock4ControllerWin {
 public:
  enum ControllerType {
    UNKNOWN_CONTROLLER,
    DUALSHOCK4_CONTROLLER,
    DUALSHOCK4_SLIM_CONTROLLER
  };

  explicit Dualshock4ControllerWin(HANDLE device_handle);
  virtual ~Dualshock4ControllerWin();

  static bool IsDualshock4(int vendor_id, int product_id);

  void PlayEffect(
      mojom::GamepadHapticEffectType,
      mojom::GamepadEffectParametersPtr,
      mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback);

  void ResetVibration(
      mojom::GamepadHapticsManager::ResetVibrationActuatorCallback);

 private:
  static ControllerType ControllerTypeFromDeviceIds(int vendor_id,
                                                    int product_id);

  void PlayDualRumbleEffect(double duration,
                            double start_delay,
                            double strong_magnitude,
                            double weak_magnitude);
  void StartVibration(int sequence_id,
                      double duration,
                      double strong_magnitude,
                      double weak_magnitude);
  void StopVibration(int sequence_id);
  void SetVibration(double strong_magnitude, double weak_magnitude);

  int sequence_id_;
  base::win::ScopedHandle hid_handle_;
  mojom::GamepadHapticsManager::PlayVibrationEffectOnceCallback
      pending_callback_;
};

}  // namespace device

#endif  // DEVICE_GAMEPAD_DUALSHOCK4_CONTROLLER_WIN_
