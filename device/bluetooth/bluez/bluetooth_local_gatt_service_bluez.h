// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUEZ_BLUETOOTH_LOCAL_GATT_SERVICE_BLUEZ_H_
#define DEVICE_BLUETOOTH_BLUEZ_BLUETOOTH_LOCAL_GATT_SERVICE_BLUEZ_H_

#include <string>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "device/bluetooth/bluetooth_local_gatt_service.h"
#include "device/bluetooth/bluetooth_uuid.h"
#include "device/bluetooth/bluez/bluetooth_gatt_service_bluez.h"

namespace device {

class BluetoothAdapter;
class BluetoothDevice;

}  // namespace device

namespace bluez {

class BluetoothAdapterBlueZ;
class BluetoothDeviceBlueZ;

// The BluetoothLocalGattServiceBlueZ class implements BluetootGattService
// for local GATT services for platforms that use BlueZ.
class BluetoothLocalGattServiceBlueZ
    : public BluetoothGattServiceBlueZ,
      public device::BluetoothLocalGattService {
 public:
  // device::BluetoothGattService overrides.
  device::BluetoothUUID GetUUID() const override;
  bool IsPrimary() const override;

  // device::BluetoothLocalGattService overrides.
  static base::WeakPtr<device::BluetoothLocalGattService> Create(
      device::BluetoothAdapter* adapter,
      const device::BluetoothUUID& uuid,
      bool is_primary,
      BluetoothLocalGattService* included_service,
      BluetoothLocalGattService::Delegate* delegate);
  void Register(const base::Closure& callback,
                const ErrorCallback& error_callback) override;
  void Unregister(const base::Closure& callback,
                  const ErrorCallback& error_callback) override;

  BluetoothLocalGattServiceBlueZ(
      BluetoothAdapterBlueZ* adapter,
      const device::BluetoothUUID& uuid,
      bool is_primary,
      device::BluetoothLocalGattService::Delegate* delegate);
  ~BluetoothLocalGattServiceBlueZ() override;

 private:
  // Called by dbus:: on unsuccessful completion of a request to register a
  // local service.
  void OnRegistrationError(const ErrorCallback& error_callback,
                           const std::string& error_name,
                           const std::string& error_message);

  BluetoothAdapterBlueZ* adapter_;
  const device::BluetoothUUID& uuid_;
  bool is_primary_;
  device::BluetoothLocalGattService::Delegate* delegate_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<BluetoothLocalGattServiceBlueZ> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothLocalGattServiceBlueZ);
};

}  // namespace bluez

#endif  // DEVICE_BLUETOOTH_BLUEZ_BLUETOOTH_LOCAL_GATT_SERVICE_BLUEZ_H_
