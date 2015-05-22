// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PROXIMITY_AUTH_BLUETOOTH_LOW_ENERGY_CONNECTION_H
#define COMPONENTS_PROXIMITY_AUTH_BLUETOOTH_LOW_ENERGY_CONNECTION_H

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "components/proximity_auth/ble/bluetooth_low_energy_characteristics_finder.h"
#include "components/proximity_auth/ble/fake_wire_message.h"
#include "components/proximity_auth/ble/remote_attribute.h"
#include "components/proximity_auth/connection.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_gatt_characteristic.h"
#include "device/bluetooth/bluetooth_gatt_notify_session.h"
#include "device/bluetooth/bluetooth_uuid.h"

namespace proximity_auth {

// Represents a connection with a remote device over Bluetooth low energy. The
// connection is a persistent bidirectional channel for sending and receiving
// wire messages. The remote device is the peripheral mode and the service
// contains two characteristics: one to send data and another to receive it.
//
// The connection flow is described below.
//
//          Discover Reader and Writer Characteristics
//                              |
//                              |
//                              |
//                    Start Notify Session
//                              |
//                              |
//                              |
//     Write kInviteToConnectSignal to Writer Characteristic
//                              |
//                              |
//                              |
//    Read kInvitationResponseSignal from Reader Characteristic
//                              |
//                              |
//                              |
//           Proximity Auth Connection Established
class BluetoothLowEnergyConnection : public Connection,
                                     public device::BluetoothAdapter::Observer {
 public:
  enum class ControlSignal : uint32 {
    kInviteToConnectSignal = 0,
    kInvitationResponseSignal = 1,
    kSendSignal = 2,
    kDisconnectSignal = 3,
  };

  // Constructs a Bluetooth low energy connection to the service with
  // |remote_service_| on the |remote_device|. The |adapter| must be already
  // initaalized and ready. The GATT connection may alreaady be established and
  // pass through |gatt_connection|. If |gatt_connection| is NULL, a subsequent
  // call to Connect() must be made.
  BluetoothLowEnergyConnection(
      const RemoteDevice& remote_device,
      scoped_refptr<device::BluetoothAdapter> adapter,
      const device::BluetoothUUID remote_service_uuid,
      const device::BluetoothUUID to_peripheral_char_uuid,
      const device::BluetoothUUID from_peripheral_char_uuid,
      scoped_ptr<device::BluetoothGattConnection> gatt_connection);

  ~BluetoothLowEnergyConnection() override;

  // proximity_auth::Connection
  void Connect() override;
  void Disconnect() override;

 protected:
  // The sub-state of a proximity_auth::BluetoothLowEnergyConnection class
  // extends the IN_PROGRESS state of proximity_auth::Connection::Status.
  enum class SubStatus {
    DISCONNECTED,
    WAITING_GATT_CONNECTION,
    GATT_CONNECTION_ESTABLISHED,
    WAITING_CHARACTERISTICS,
    CHARACTERISTICS_FOUND,
    WAITING_NOTIFY_SESSION,
    NOTIFY_SESSION_READY,
    WAITING_RESPONSE_SIGNAL,
    CONNECTED,
  };

  void SetSubStatus(SubStatus status);
  SubStatus sub_status() { return sub_status_; }

  // proximity_auth::Connection
  void SendMessageImpl(scoped_ptr<WireMessage> message) override;
  scoped_ptr<WireMessage> DeserializeWireMessage(
      bool* is_incomplete_message) override;

  // device::BluetoothAdapter::Observer
  void DeviceRemoved(device::BluetoothAdapter* adapter,
                     device::BluetoothDevice* device) override;
  void GattCharacteristicValueChanged(
      device::BluetoothAdapter* adapter,
      device::BluetoothGattCharacteristic* characteristic,
      const std::vector<uint8>& value) override;

 private:
  // Called when a GATT connection is created or received by the constructor.
  void OnGattConnectionCreated(
      scoped_ptr<device::BluetoothGattConnection> gatt_connection);

  // Callback called when there is an error creating the connection.
  void OnCreateGattConnectionError(
      device::BluetoothDevice::ConnectErrorCode error_code);

  // Callback called when |to_peripheral_char_| and |from_peripheral_char_| were
  // found.
  void OnCharacteristicsFound(const RemoteAttribute& service,
                              const RemoteAttribute& to_peripheral_char,
                              const RemoteAttribute& from_peripheral_char);

  // Callback called there was an error finding the characteristics.
  void OnCharacteristicsFinderError(
      const RemoteAttribute& to_peripheral_char,
      const RemoteAttribute& from_peripheral_char);

  // Starts a notify session for |from_peripheral_char_| when ready
  // (SubStatus::CHARACTERISTICS_FOUND).
  void StartNotifySession();

  // Called when a notification session is successfully started for
  // |from_peripheral_char_| characteristic.
  void OnNotifySessionStarted(
      scoped_ptr<device::BluetoothGattNotifySession> notify_session);

  // Called when there is an error starting a notification session for
  // |from_peripheral_char_| characteristic.
  void OnNotifySessionError(device::BluetoothGattService::GattErrorCode);

  // Stops |notify_session_|.
  void StopNotifySession();

  // Sends an invite to connect signal to the peripheral if when ready
  // (SubStatus::NOTIFY_SESSION_READY).
  void SendInviteToConnectSignal();

  // Completes and updates the status accordingly.
  void CompleteConnection();

  // Called when there is an error writing to the remote characteristic
  // |to_peripheral_char_|.
  void OnWriteRemoteCharacteristicError(
      device::BluetoothGattService::GattErrorCode error);

  // Returns the Bluetooth address of the remote device.
  const std::string& GetRemoteDeviceAddress();

  // Returns the device corresponding to |remote_device_address_|.
  device::BluetoothDevice* GetRemoteDevice();

  // Returns the service corresponding to |remote_service_| in the current
  // device.
  device::BluetoothGattService* GetRemoteService();

  // Returns the characteristic corresponding to |identifier| in the current
  // service.
  device::BluetoothGattCharacteristic* GetGattCharacteristic(
      const std::string& identifier);

  // Convert the first 4 bytes from a byte vector to a uint32.
  uint32 ToUint32(const std::vector<uint8>& bytes);

  // Convert an uint32 to a byte array.
  const std::string ToString(const uint32 value);

  // The Bluetooth adapter over which the Bluetooth connection will be made.
  scoped_refptr<device::BluetoothAdapter> adapter_;

  // Remote service the |connection_| was established with.
  RemoteAttribute remote_service_;

  // Characteristic used to send data to the remote device.
  RemoteAttribute to_peripheral_char_;

  // Characteristic used to receive data from the remote device.
  RemoteAttribute from_peripheral_char_;

  // The GATT connection with the remote device.
  scoped_ptr<device::BluetoothGattConnection> connection_;

  // The characteristics finder for remote device.
  scoped_ptr<BluetoothLowEnergyCharacteristicsFinder> characteristic_finder_;

  // The notify session for |from_peripheral_char|.
  scoped_ptr<device::BluetoothGattNotifySession> notify_session_;

  // Internal connection status
  SubStatus sub_status_;

  // True if it's receiving bytes from |from_peripheral_char_|. This is set
  // after a ControlSignal::kSendSignal is received.
  bool receiving_bytes_;

  // Stores when the instace was created.
  base::TimeTicks start_time_;

  base::WeakPtrFactory<BluetoothLowEnergyConnection> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothLowEnergyConnection);
};

}  // namespace proximity_auth

#endif  // COMPONENTS_PROXIMITY_AUTH_BLUETOOTH_LOW_ENERGY_CONNECTION_H
