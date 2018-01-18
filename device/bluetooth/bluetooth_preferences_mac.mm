// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_preferences_mac.h"

#import <IOBluetooth/IOBluetooth.h>

// Undocumented IOBluetooth preference API:
extern "C" {
void IOBluetoothPreferenceSetControllerPowerState(int state);
};

namespace device {

BluetoothPreferencesMac::BluetoothPreferencesMac() = default;

BluetoothPreferencesMac::~BluetoothPreferencesMac() = default;

void BluetoothPreferencesMac::SetControllerPowerState(bool state) {
  IOBluetoothPreferenceSetControllerPowerState(state);
}

}  // namespace device
