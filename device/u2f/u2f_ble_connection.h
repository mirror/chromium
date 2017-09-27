// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_U2F_U2F_BLE_CONNECTION_
#define DEVICE_U2F_U2F_BLE_CONNECTION_

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_gatt_service.h"

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

namespace device {

class BluetoothDevice;
class BluetoothGattConnection;
class BluetoothGattNotifySession;
class BluetoothRemoteGattCharacteristic;

// A connection to the U2F service of an authenticator over BLE. Detailed
// specification of the BLE device can be found here:
// https://fidoalliance.org/specs/fido-u2f-v1.2-ps-20170411/fido-u2f-bt-protocol-v1.2-ps-20170411.html#h2_gatt-service-description
//
// TODO(pkalinnikov): Device discovery and iteration.
// TODO(pkalinnikov): Add pairing.
// TODO(pkalinnikov): Listen to disconnect events?
class U2fBleConnection : public BluetoothAdapter::Observer {
 public:
  using ConnectCallback = base::Callback<void(bool)>;
  using WriteCallback = base::Callback<void(bool)>;
  using ReadCallback = base::Callback<void(std::vector<uint8_t>)>;
  using ControlPointLengthCallback = base::Callback<void(uint16_t)>;

  U2fBleConnection(std::string device_address, ConnectCallback, ReadCallback);
  ~U2fBleConnection() override;

  const std::string& address() const { return address_; }

  void ReadControlPointLength(ControlPointLengthCallback callback);
  // TODO(pkalinnikov): Read Service Revision.

  void Write(const std::vector<uint8_t>& data, WriteCallback);

 private:
  using This = U2fBleConnection;

  void GetAdapterCallback(scoped_refptr<BluetoothAdapter> adapter);
  void OnSetPowered();
  void OnConnected();
  void OnGattConnected(std::unique_ptr<BluetoothGattConnection> connection);

  // BluetoothAdapter::Observer:
  void GattServicesDiscovered(BluetoothAdapter* adapter,
                              BluetoothDevice* device) override;

  void ConnectToU2fService();

  void NotifySessionCallback(
      std::unique_ptr<BluetoothGattNotifySession> session);

  void GattErrorCallback(BluetoothGattService::GattErrorCode code);

  // BluetoothAdapter::Observer:
  void GattCharacteristicValueChanged(
      BluetoothAdapter* adapter,
      BluetoothRemoteGattCharacteristic* characteristic,
      const std::vector<uint8_t>& value) override;

  std::string address_;
  ConnectCallback connect_callback_;
  ReadCallback read_callback_;

  scoped_refptr<BluetoothAdapter> adapter_;
  BluetoothDevice* device_ = nullptr;
  std::unique_ptr<BluetoothGattConnection> gatt_connection_;

  BluetoothRemoteGattCharacteristic* control_point_length_;
  BluetoothRemoteGattCharacteristic* control_point_;
  BluetoothRemoteGattCharacteristic* status_;

  std::unique_ptr<BluetoothGattNotifySession> status_notification_session_;

  base::WeakPtrFactory<U2fBleConnection> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(U2fBleConnection);
};

}  // namespace device

#endif  // DEVICE_U2F_U2F_BLE_CONNECTION_
