// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/proximity_auth/pollux/ble_scanner.h"

#include <memory>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/cryptauth/background_eid_generator.h"
#include "components/cryptauth/ble/bluetooth_low_energy_weave_client_connection.h"
#include "components/cryptauth/bluetooth_throttler.h"
#include "components/cryptauth/connection.h"
#include "components/cryptauth/raw_eid_generator.h"
#include "components/proximity_auth/logging/logging.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_common.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_discovery_session.h"
#include "device/bluetooth/bluetooth_uuid.h"

using device::BluetoothAdapter;
using device::BluetoothDevice;
using device::BluetoothGattConnection;
using device::BluetoothDiscoveryFilter;

namespace pollux {
namespace {

const char kAdvertisementUUID[] = "0000fe51-0000-1000-8000-00805f9b34fb";
const char kBLEGattServiceUUID[] = "b3b7e28e-a000-3e17-bd86-6e97b9e28c11";
const int kRestartDiscoveryOnErrorDelaySeconds = 2;

class StubBluetoothThrottler : public cryptauth::BluetoothThrottler {
 public:
  StubBluetoothThrottler() {}
  virtual ~StubBluetoothThrottler() {}

  // cryptauth::BluetoothThrottler
  base::TimeDelta GetDelay() const override {
    return base::TimeDelta::FromSeconds(0);
  }

  void OnConnection(cryptauth::Connection* connection) override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(StubBluetoothThrottler);
};

}  // namespace

BleScanner::BleScanner(scoped_refptr<device::BluetoothAdapter> adapter,
                       const std::string& remote_eid)
    : adapter_(adapter),
      remote_eid_(remote_eid),
      bluetooth_throttler_(new StubBluetoothThrottler()),
      weak_ptr_factory_(this) {}

BleScanner::~BleScanner() {
  if (discovery_session_) {
    StopDiscoverySession();
  }

  if (connection_) {
    connection_->RemoveObserver(this);
    connection_.reset();
  }

  if (adapter_) {
    adapter_->RemoveObserver(this);
    adapter_ = NULL;
  }
}

void BleScanner::Find(const cryptauth::ConnectionFinder::ConnectionCallback&
                          connection_callback) {
  if (!device::BluetoothAdapterFactory::IsBluetoothSupported()) {
    PA_LOG(WARNING) << "Bluetooth is unsupported on this platform. Aborting.";
    return;
  }
  PA_LOG(INFO) << "Finding connection";

  connection_callback_ = connection_callback;

  adapter_->AddObserver(this);
  StartDiscoverySession();
}

// It's not necessary to observe |AdapterPresentChanged| too. When |adapter_| is
// present, but not powered, it's not possible to scan for new devices.
void BleScanner::AdapterPoweredChanged(BluetoothAdapter* adapter,
                                       bool powered) {
  DCHECK_EQ(adapter_.get(), adapter);
  PA_LOG(INFO) << "Adapter powered: " << powered;

  // Important: do not rely on |adapter->IsDiscoverying()| to verify if there is
  // an active discovery session. We need to create our own with an specific
  // filter.
  if (powered && (!discovery_session_ || !discovery_session_->IsActive()))
    StartDiscoverySession();
}

void BleScanner::DeviceAdded(BluetoothAdapter* adapter,
                             BluetoothDevice* device) {
  DCHECK_EQ(adapter_.get(), adapter);
  DCHECK(device);

  // Note: Only consider |device| when it was actually added/updated during a
  // scanning, otherwise the device is stale and the GATT connection will fail.
  // For instance, when |adapter_| change status from unpowered to powered,
  // |DeviceAdded| is called for each paired |device|.
  if (adapter_->IsPowered() && discovery_session_ &&
      discovery_session_->IsActive())
    HandleDeviceUpdated(device);
}

void BleScanner::DeviceChanged(BluetoothAdapter* adapter,
                               BluetoothDevice* device) {
  DCHECK_EQ(adapter_.get(), adapter);
  DCHECK(device);

  // Note: Only consider |device| when it was actually added/updated during a
  // scanning, otherwise the device is stale and the GATT connection will fail.
  // For instance, when |adapter_| change status from unpowered to powered,
  // |DeviceAdded| is called for each paired |device|.
  if (adapter_->IsPowered() && discovery_session_ &&
      discovery_session_->IsActive())
    HandleDeviceUpdated(device);
}

void BleScanner::HandleDeviceUpdated(BluetoothDevice* device) {
  // Ensuring only one call to |CreateConnection()| is made. A new |connection_|
  // can be created only when the previous one disconnects, triggering a call to
  // |OnConnectionStatusChanged|.
  if (connection_)
    return;

  if (IsRightDevice(device)) {
    PA_LOG(INFO) << "Connecting to device " << device->GetAddress();
    connection_ = CreateConnection(device->GetAddress());
    connection_->AddObserver(this);
    connection_->Connect();

    StopDiscoverySession();
  }
}

bool BleScanner::IsRightDevice(BluetoothDevice* device) {
  if (!device)
    return false;

  device::BluetoothUUID advertisement_uuid(kAdvertisementUUID);
  const std::vector<uint8_t>* service_data =
      device->GetServiceDataForUUID(advertisement_uuid);
  if (!service_data)
    return false;

  PA_LOG(INFO) << "Attempting to resolve " << device->GetAddress();
  std::string service_data_string(service_data->begin(), service_data->end());
  return service_data_string == remote_eid_;
}

void BleScanner::OnDiscoverySessionStarted(
    std::unique_ptr<device::BluetoothDiscoverySession> discovery_session) {
  PA_LOG(INFO) << "Discovery session started";
  discovery_session_ = std::move(discovery_session);
}

void BleScanner::OnStartDiscoverySessionError() {
  PA_LOG(WARNING) << "Error starting discovery session, restarting in "
                  << kRestartDiscoveryOnErrorDelaySeconds << " seconds.";
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&BleScanner::RestartDiscoverySessionAsync,
                 weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromSeconds(kRestartDiscoveryOnErrorDelaySeconds));
}

void BleScanner::StartDiscoverySession() {
  DCHECK(adapter_);
  if (discovery_session_ && discovery_session_->IsActive()) {
    PA_LOG(INFO) << "Discovery session already active";
    return;
  }

  // Discover only low energy (LE) devices.
  std::unique_ptr<BluetoothDiscoveryFilter> filter(
      new BluetoothDiscoveryFilter(device::BLUETOOTH_TRANSPORT_LE));

  adapter_->StartDiscoverySessionWithFilter(
      std::move(filter),
      base::Bind(&BleScanner::OnDiscoverySessionStarted,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&BleScanner::OnStartDiscoverySessionError,
                 weak_ptr_factory_.GetWeakPtr()));
}

void BleScanner::StopDiscoverySession() {
  PA_LOG(INFO) << "Stopping discovery session";
  // Destroying the discovery session also stops it.
  discovery_session_.reset();
}

std::unique_ptr<cryptauth::Connection> BleScanner::CreateConnection(
    const std::string& device_address) {
  cryptauth::RemoteDevice remote_device;
  remote_device.public_key = "pollux_device";
  return cryptauth::weave::BluetoothLowEnergyWeaveClientConnection::Factory::
      NewInstance(remote_device, device_address, adapter_,
                  device::BluetoothUUID(kBLEGattServiceUUID),
                  bluetooth_throttler_.get());
}

void BleScanner::OnConnectionStatusChanged(
    cryptauth::Connection* connection,
    cryptauth::Connection::Status old_status,
    cryptauth::Connection::Status new_status) {
  DCHECK_EQ(connection, connection_.get());
  PA_LOG(INFO) << "OnConnectionStatusChanged: " << old_status << " -> "
               << new_status;

  if (!connection_callback_.is_null() && connection_->IsConnected()) {
    adapter_->RemoveObserver(this);
    connection_->RemoveObserver(this);

    // If we invoke the callback now, the callback function may install its own
    // observer to |connection_|. Because we are in the ConnectionObserver
    // callstack, this new observer will receive this connection event.
    // Therefore, we need to invoke the callback or restart discovery
    // asynchronously.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&BleScanner::InvokeCallbackAsync,
                              weak_ptr_factory_.GetWeakPtr()));
  } else if (old_status == cryptauth::Connection::IN_PROGRESS) {
    PA_LOG(WARNING) << "Connection failed. Retrying.";
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&BleScanner::RestartDiscoverySessionAsync,
                              weak_ptr_factory_.GetWeakPtr()));
  }
}

void BleScanner::RestartDiscoverySessionAsync() {
  PA_LOG(INFO) << "Restarting discovery session.";
  connection_.reset();
  if (!discovery_session_ || !discovery_session_->IsActive())
    StartDiscoverySession();
}

void BleScanner::InvokeCallbackAsync() {
  connection_callback_.Run(std::move(connection_));
}

}  // namespace pollux
