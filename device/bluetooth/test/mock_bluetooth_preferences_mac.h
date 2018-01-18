// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_TEST_MOCK_BLUETOOTH_PREFERENCES_MAC_H_
#define DEVICE_BLUETOOTH_TEST_MOCK_BLUETOOTH_PREFERENCES_MAC_H_

#include "device/bluetooth/bluetooth_preferences_mac.h"

namespace device {

class BluetoothAdapterMac;

class MockBluetoothPreferencesMac : public BluetoothPreferencesMac {
 public:
  explicit MockBluetoothPreferencesMac(BluetoothAdapterMac* adapter_mac);
  ~MockBluetoothPreferencesMac() override;

  // BluetoothPreferencesMac overrides:
  void SetControllerPowerState(bool powered) override;

 private:
  BluetoothAdapterMac* adapter_mac_;
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_TEST_MOCK_BLUETOOTH_PREFERENCES_MAC_H_
