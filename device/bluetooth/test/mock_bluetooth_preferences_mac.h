// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_TEST_MOCK_BLUETOOTH_PREFERENCES_MAC_H_
#define DEVICE_BLUETOOTH_TEST_MOCK_BLUETOOTH_PREFERENCES_MAC_H_

#include "device/bluetooth/bluetooth_preferences_mac.h"
#include "device/bluetooth/test/mock_bluetooth_central_manager_mac.h"

namespace device {

class MockBluetoothPreferencesMac : public BluetoothPreferencesMac {
 public:
  explicit MockBluetoothPreferencesMac(
      MockCentralManager* mock_central_manager);
  ~MockBluetoothPreferencesMac() override;

  // BluetoothPreferecesMac overrides:
  void SetControllerPowerState(bool state) override;

 private:
  MockCentralManager* mock_central_manager_;
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_TEST_MOCK_BLUETOOTH_PREFERENCES_MAC_H_
