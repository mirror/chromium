// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GAMEPAD_SWITCH_PRO_CONTROLLER_BASE_
#define DEVICE_GAMEPAD_SWITCH_PRO_CONTROLLER_BASE_

#include "device/gamepad/abstract_haptic_gamepad.h"

namespace device {

class SwitchProControllerBase : public AbstractHapticGamepad {
 public:
  SwitchProControllerBase() = default;
  ~SwitchProControllerBase() override;

  static bool IsSwitchPro(int vendor_id, int product_id);

  void SendConnectionStatusQuery();
  void SendHandshake();
  void SendForceUsbHid(bool enable);
  void SetVibration(double strong_magnitude, double weak_magnitude) override;

 private:
  virtual void WriteOutputReport(void* report, size_t report_length) {}

  uint32_t counter_ = 0;
};

}  // namespace device

#endif  // DEVICE_GAMEPAD_SWITCH_PRO_CONTROLLER_BASE_
