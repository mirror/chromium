// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/test/mock_bluetooth_preferences_mac.h"

#include "base/bind.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "device/bluetooth/bluetooth_adapter_mac.h"
#include "device/bluetooth/test/mock_bluetooth_central_manager_mac.h"

namespace device {

MockBluetoothPreferencesMac::MockBluetoothPreferencesMac(
    BluetoothAdapterMac* adapter_mac)
    : adapter_mac_(adapter_mac) {}

MockBluetoothPreferencesMac::~MockBluetoothPreferencesMac() = default;

void MockBluetoothPreferencesMac::SetControllerPowerState(bool powered) {
  // We are posting a task so that the state only gets updated in the next cycle
  // and pending callbacks are not executed immediately.
  adapter_mac_->ui_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](base::WeakPtr<BluetoothAdapterMac> adapter_mac, bool powered) {
            // Guard against deletion of the adapter.
            if (!adapter_mac)
              return;

            MockCentralManager* mock_central_manager =
                (id)adapter_mac->GetCentralManager();
            [mock_central_manager
                setState:powered ? CBCentralManagerStatePoweredOn
                                 : CBCentralManagerStatePoweredOff];
            [mock_central_manager.delegate
                centralManagerDidUpdateState:(id)mock_central_manager];
            adapter_mac->NotifyAdapterPoweredChanged(powered);
          },
          adapter_mac_->weak_ptr_factory_.GetWeakPtr(), powered));
}

}  // namespace device
