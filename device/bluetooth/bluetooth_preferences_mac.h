// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUETOOTH_PREFERENCES_MAC_H_
#define DEVICE_BLUETOOTH_BLUETOOTH_PREFERENCES_MAC_H_

#include "device/bluetooth/bluetooth_export.h"

namespace device {

// BluetoothPreferencesMac provides a wrapper around private IOBluetooth C APIs
// controlling the state of Bluetooth preferences. These wrappers are declared
// virtual, so that they can be replaced with fakes in tests.
class DEVICE_BLUETOOTH_EXPORT BluetoothPreferencesMac {
 public:
  BluetoothPreferencesMac();
  virtual ~BluetoothPreferencesMac();

  virtual void SetControllerPowerState(bool state);
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_BLUETOOTH_PREFERENCES_MAC_H_
