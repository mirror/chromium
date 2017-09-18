// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_U2F_U2F_BLE_DISCOVERY_H_
#define DEVICE_U2F_U2F_BLE_DISCOVERY_H_

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "device/bluetooth/bluetooth_adapter.h"

#include <memory>
#include <set>
#include <string>

namespace device {

class BluetoothDiscoverySession;

class U2fBleDiscovery : public BluetoothAdapter::Observer {
 public:
  U2fBleDiscovery();
  ~U2fBleDiscovery() override;

  void Start();
  void Stop();

  const std::set<std::string>& devices() const { return devices_; }

 private:
  void GetAdapterCallback(scoped_refptr<BluetoothAdapter> adapter);
  void OnSetPowered();
  void DiscoverySessionStarted(std::unique_ptr<BluetoothDiscoverySession>);

  // BluetoothAdapter::Observer:
  void DeviceAdded(BluetoothAdapter* adapter, BluetoothDevice* device) override;
  void DeviceRemoved(BluetoothAdapter* adapter,
                     BluetoothDevice* device) override;

  scoped_refptr<BluetoothAdapter> adapter_;
  std::unique_ptr<BluetoothDiscoverySession> discovery_session_;

  std::set<std::string> devices_;

  base::WeakPtrFactory<U2fBleDiscovery> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(U2fBleDiscovery);
};

}  // namespace device

#endif  // DEVICE_U2F_U2F_BLE_DISCOVERY_H_
