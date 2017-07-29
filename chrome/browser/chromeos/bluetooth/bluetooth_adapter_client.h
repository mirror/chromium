// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_BLUETOOTH_BLUETOOTH_ADAPTER_CLIENT_H_
#define CHROME_BROWSER_CHROMEOS_BLUETOOTH_BLUETOOTH_ADAPTER_CLIENT_H_

#include "ash/public/interfaces/bluetooth_adapter.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

// Listens to bluetooth adapter power change initiated from ash, and save
// the adapter power state to local state or user preference.
class BluetoothAdapterClient
    : public NON_EXPORTED_BASE(ash::mojom::BluetoothAdapterClient) {
 public:
  BluetoothAdapterClient();
  ~BluetoothAdapterClient() override;

  void OnAdapterPowerSet(bool enabled) override;

 private:
  ash::mojom::BluetoothAdapterControllerPtr bluetooth_adapter_controller_;

  mojo::Binding<ash::mojom::BluetoothAdapterClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothAdapterClient);
};

#endif  // CHROME_BROWSER_CHROMEOS_BLUETOOTH_BLUETOOTH_ADAPTER_CLIENT_H_
