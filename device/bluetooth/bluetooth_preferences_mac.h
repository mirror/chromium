// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUETOOTH_PREFERENCES_MAC_H_
#define DEVICE_BLUETOOTH_BLUETOOTH_PREFERENCES_MAC_H_

namespace device {

class BluetoothPreferencesMac {
 public:
  BluetoothPreferencesMac();
  virtual ~BluetoothPreferencesMac();

  virtual void SetControllerPowerState(bool state);
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_BLUETOOTH_PREFERENCES_MAC_H_
