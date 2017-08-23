// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PROXIMITY_AUTH_POLLUX_BLE_SCANNER_H_
#define COMPONENTS_PROXIMITY_AUTH_POLLUX_BLE_SCANNER_H_

#include <memory>
#include <set>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "components/cryptauth/background_eid_generator.h"
#include "components/cryptauth/bluetooth_throttler.h"
#include "components/cryptauth/connection.h"
#include "components/cryptauth/connection_finder.h"
#include "components/cryptauth/connection_observer.h"
#include "components/cryptauth/remote_beacon_seed_fetcher.h"
#include "components/cryptauth/remote_device.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_discovery_session.h"
#include "device/bluetooth/bluetooth_gatt_connection.h"

namespace pollux {

class BleScanner : public cryptauth::ConnectionObserver,
                   public device::BluetoothAdapter::Observer {
 public:
  BleScanner(scoped_refptr<device::BluetoothAdapter> adapter,
             const std::string& remote_eid);

  virtual ~BleScanner();

  // Finds a connection to the remote device.
  void Find(const cryptauth::ConnectionFinder::ConnectionCallback&
                connection_callback);

  // cryptauth::ConnectionObserver:
  void OnConnectionStatusChanged(
      cryptauth::Connection* connection,
      cryptauth::Connection::Status old_status,
      cryptauth::Connection::Status new_status) override;

  // device::BluetoothAdapter::Observer:
  void AdapterPoweredChanged(device::BluetoothAdapter* adapter,
                             bool powered) override;
  void DeviceAdded(device::BluetoothAdapter* adapter,
                   device::BluetoothDevice* device) override;
  void DeviceChanged(device::BluetoothAdapter* adapter,
                     device::BluetoothDevice* device) override;

 protected:
  // Creates a proximity_auth::Connection with the device given by
  // |device_address|. Exposed for testing.
  virtual std::unique_ptr<cryptauth::Connection> CreateConnection(
      const std::string& device_address);

  // Checks if |device| is advertising the right EID.
  virtual bool IsRightDevice(device::BluetoothDevice* device);

 private:
  // Checks if |remote_device| contains |remote_service_uuid| and creates a
  // connection in that case.
  void HandleDeviceUpdated(device::BluetoothDevice* remote_device);

  // Callback called when a new discovery session is started.
  void OnDiscoverySessionStarted(
      std::unique_ptr<device::BluetoothDiscoverySession> discovery_session);

  // Callback called when there is an error starting a new discovery session.
  void OnStartDiscoverySessionError();

  // Starts a discovery session for |adapter_|.
  void StartDiscoverySession();

  // Stops the discovery session given by |discovery_session_|.
  void StopDiscoverySession();

  // Restarts the discovery session after creating |connection_| fails.
  void RestartDiscoverySessionAsync();

  // Used to invoke |connection_callback_| asynchronously, decoupling the
  // callback invocation from the ConnectionObserver callstack.
  void InvokeCallbackAsync();

  // The Bluetooth adapter over which the Bluetooth connection will be made.
  scoped_refptr<device::BluetoothAdapter> adapter_;

  std::string remote_eid_;

  std::unique_ptr<cryptauth::BluetoothThrottler> bluetooth_throttler_;

  // The discovery session associated to this object.
  std::unique_ptr<device::BluetoothDiscoverySession> discovery_session_;

  // The connection with |remote_device|.
  std::unique_ptr<cryptauth::Connection> connection_;

  // Callback called when the connection is established.
  cryptauth::ConnectionFinder::ConnectionCallback connection_callback_;

  base::WeakPtrFactory<BleScanner> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BleScanner);
};

}  // namespace pollux

#endif  // COMPONENTS_PROXIMITY_AUTH_POLLUX_BLE_SCANNER_H_
