// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/test/mock_bluetooth_preferences_mac.h"

namespace device {

MockBluetoothPreferencesMac::MockBluetoothPreferencesMac(
    MockCentralManager* mock_central_manager)
    : mock_central_manager_(mock_central_manager) {}

MockBluetoothPreferencesMac::~MockBluetoothPreferencesMac() = default;

void MockBluetoothPreferencesMac::SetControllerPowerState(bool state) {
  const bool current_state =
      [mock_central_manager_ state] == CBCentralManagerStatePoweredOn;
  if (current_state == state)
    return;

  [mock_central_manager_ setState:state ? CBCentralManagerStatePoweredOn
                                        : CBCentralManagerStatePoweredOff];
  [mock_central_manager_.delegate
      centralManagerDidUpdateState:(id)mock_central_manager_];
}

}  // namespace device
