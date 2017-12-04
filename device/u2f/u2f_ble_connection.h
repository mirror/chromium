// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_U2F_U2F_BLE_CONNECTION_H_
#define DEVICE_U2F_U2F_BLE_CONNECTION_H_

#include <stdint.h>

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "device/bluetooth/adapter_factory.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_gatt_service.h"
#include "device/bluetooth/public/interfaces/device.mojom.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace device {

class BluetoothGattNotifySession;

// A connection to the U2F service of an authenticator over BLE. Detailed
// specification of the BLE device can be found here:
// https://fidoalliance.org/specs/fido-u2f-v1.2-ps-20170411/fido-u2f-bt-protocol-v1.2-ps-20170411.html#h2_gatt-service-description
//
// TODO(crbug.com/763303): Add pairing.
class U2fBleConnection : public BluetoothAdapter::Observer {
 public:
  enum class ServiceRevision {
    VERSION_1_0,
    VERSION_1_1,
    VERSION_1_2,
  };

  using ConnectCallback = base::RepeatingCallback<void(bool)>;
  using WriteCallback = base::OnceCallback<void(bool)>;
  using ReadCallback = base::RepeatingCallback<void(std::vector<uint8_t>)>;
  using ControlPointLengthCallback =
      base::OnceCallback<void(base::Optional<uint16_t>)>;
  using ServiceRevisionsCallback =
      base::OnceCallback<void(std::set<ServiceRevision>)>;

  U2fBleConnection(std::string device_address,
                   ConnectCallback connect_callback,
                   ReadCallback read_callback);
  ~U2fBleConnection() override;

  const std::string& address() const { return address_; }

  void ReadControlPointLength(ControlPointLengthCallback callback);
  void ReadServiceRevisions(ServiceRevisionsCallback callback);

  void Write(const std::vector<uint8_t>& data, WriteCallback);
  void WriteServiceRevision(ServiceRevision service_revision,
                            WriteCallback callback);

 private:
  void OnGetAdapter(bluetooth::mojom::AdapterPtr adapter);
  void OnConnectToDevice(bluetooth::mojom::ConnectResult result,
                         bluetooth::mojom::DevicePtr device);
  void OnGetServices(std::vector<bluetooth::mojom::ServiceInfoPtr> services);
  void OnGetCharacteristics(
      base::Optional<std::vector<bluetooth::mojom::CharacteristicInfoPtr>>
          characteristics);
  void OnConnectionError();

  // BluetoothAdapter::Observer:
  void DeviceAdded(BluetoothAdapter* adapter, BluetoothDevice* device) override;
  void DeviceAddressChanged(BluetoothAdapter* adapter,
                            BluetoothDevice* device,
                            const std::string& old_address) override;

  // Since the Mojo based Bluetooth API does not support starting a notify
  // session, the following legacy implementation is necessary to get updates
  // from the status characteristic.
  // TODO(crbug.com/763303): Clean this up once StartNotifySession is supported
  // in Mojo.
  void GattCharacteristicValueChanged(
      BluetoothAdapter* adapter,
      BluetoothRemoteGattCharacteristic* characteristic,
      const std::vector<uint8_t>& value) override;

  void OnGetAdapterLegacy(scoped_refptr<BluetoothAdapter> adapter);

  void OnStartNotifySession(
      std::unique_ptr<BluetoothGattNotifySession> notify_session);
  void OnStartNotifySessionError(BluetoothGattService::GattErrorCode code);

  static void OnReadControlPointLength(
      ControlPointLengthCallback callback,
      bluetooth::mojom::GattResult result,
      const base::Optional<std::vector<uint8_t>>& value);

  void OnReadServiceRevision(base::OnceClosure callback,
                             bluetooth::mojom::GattResult result,
                             const base::Optional<std::vector<uint8_t>>& value);

  void OnReadServiceRevisionBitfied(
      base::OnceClosure callback,
      bluetooth::mojom::GattResult result,
      const base::Optional<std::vector<uint8_t>>& value);

  void ReturnServiceRevisions(ServiceRevisionsCallback callback);

  static void OnWrite(WriteCallback callback,
                      bluetooth::mojom::GattResult result);

  std::string address_;
  ConnectCallback connect_callback_;
  ReadCallback read_callback_;

  bluetooth::AdapterFactory factory_;
  bluetooth::mojom::AdapterPtr adapter_;
  bluetooth::mojom::DevicePtr device_;
  bluetooth::mojom::ServiceInfoPtr u2f_service_;
  bluetooth::mojom::CharacteristicInfoPtr control_point_length_;
  bluetooth::mojom::CharacteristicInfoPtr control_point_;
  bluetooth::mojom::CharacteristicInfoPtr status_;
  bluetooth::mojom::CharacteristicInfoPtr service_revision_;
  bluetooth::mojom::CharacteristicInfoPtr service_revision_bitfield_;

  std::set<ServiceRevision> service_revisions_;

  scoped_refptr<BluetoothAdapter> legacy_adapter_;
  std::unique_ptr<BluetoothGattNotifySession> notify_session_;

  base::WeakPtrFactory<U2fBleConnection> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(U2fBleConnection);
};

}  // namespace device

#endif  // DEVICE_U2F_U2F_BLE_CONNECTION_H_
