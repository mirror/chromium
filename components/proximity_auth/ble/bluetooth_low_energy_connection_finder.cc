// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/proximity_auth/ble/bluetooth_low_energy_connection_finder.h"

#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/thread_task_runner_handle.h"
#include "components/proximity_auth/ble/bluetooth_low_energy_connection.h"
#include "components/proximity_auth/connection.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_discovery_session.h"
#include "device/bluetooth/bluetooth_uuid.h"

using device::BluetoothAdapter;
using device::BluetoothDevice;
using device::BluetoothGattConnection;
using device::BluetoothDiscoveryFilter;

namespace proximity_auth {
namespace {
const int kMinDiscoveryRSSI = -100;
const int kDelayAfterGattConnectionMilliseconds = 1000;
}  // namespace

BluetoothLowEnergyConnectionFinder::BluetoothLowEnergyConnectionFinder(
    const std::string& remote_service_uuid,
    const std::string& to_peripheral_char_uuid,
    const std::string& from_peripheral_char_uuid,
    int max_number_of_tries)
    : remote_service_uuid_(device::BluetoothUUID(remote_service_uuid)),
      to_peripheral_char_uuid_(device::BluetoothUUID(to_peripheral_char_uuid)),
      from_peripheral_char_uuid_(
          device::BluetoothUUID(from_peripheral_char_uuid)),
      connected_(false),
      max_number_of_tries_(max_number_of_tries),
      delay_after_gatt_connection_(base::TimeDelta::FromMilliseconds(
          kDelayAfterGattConnectionMilliseconds)),
      weak_ptr_factory_(this) {
}

BluetoothLowEnergyConnectionFinder::~BluetoothLowEnergyConnectionFinder() {
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

void BluetoothLowEnergyConnectionFinder::Find(
    const ConnectionCallback& connection_callback) {
  if (!device::BluetoothAdapterFactory::IsBluetoothAdapterAvailable()) {
    VLOG(1) << "Bluetooth is unsupported on this platform. Aborting.";
    return;
  }
  VLOG(1) << "Finding connection";

  connection_callback_ = connection_callback;

  device::BluetoothAdapterFactory::GetAdapter(
      base::Bind(&BluetoothLowEnergyConnectionFinder::OnAdapterInitialized,
                 weak_ptr_factory_.GetWeakPtr()));
}

void BluetoothLowEnergyConnectionFinder::DeviceAdded(BluetoothAdapter* adapter,
                                                     BluetoothDevice* device) {
  DCHECK(device);
  VLOG(1) << "Device added: " << device->GetAddress();
  HandleDeviceUpdated(device);
}

void BluetoothLowEnergyConnectionFinder::DeviceChanged(
    BluetoothAdapter* adapter,
    BluetoothDevice* device) {
  DCHECK(device);
  VLOG(1) << "Device changed: " << device->GetAddress();
  HandleDeviceUpdated(device);
}

void BluetoothLowEnergyConnectionFinder::HandleDeviceUpdated(
    BluetoothDevice* device) {
  if (connected_)
    return;
  const auto& i = pending_connections_.find(device);
  if (i != pending_connections_.end()) {
    VLOG(1) << "Pending connection to device " << device->GetAddress();
    return;
  }
  if (HasService(device) && device->IsPaired()) {
    VLOG(1) << "Connecting to paired device " << device->GetAddress();
    pending_connections_.insert(device);
    CreateGattConnection(device);
  }
}

void BluetoothLowEnergyConnectionFinder::DeviceRemoved(
    BluetoothAdapter* adapter,
    BluetoothDevice* device) {
  if (connected_)
    return;

  const auto& i = pending_connections_.find(device);
  if (i != pending_connections_.end()) {
    VLOG(1) << "Remove pending connection to  " << device->GetAddress();
    pending_connections_.erase(i);
  }
}

void BluetoothLowEnergyConnectionFinder::OnAdapterInitialized(
    scoped_refptr<BluetoothAdapter> adapter) {
  VLOG(1) << "Adapter ready";

  adapter_ = adapter;
  adapter_->AddObserver(this);

  // Note: it's not possible to connect with the paired directly, as the
  // temporary MAC may not be resolved automatically (see crbug.com/495402). The
  // Bluetooth adapter will fire |OnDeviceChanged| notifications for all
  // Bluetooth Low Energy devices that are advertising.
  if (VLOG_IS_ON(1)) {
    std::vector<BluetoothDevice*> devices = adapter_->GetDevices();
    for (auto* device : devices) {
      VLOG(1) << "Ignoring device " << device->GetAddress()
              << " present when adapter was initialized.";
    }
  }

  StartDiscoverySession();
}

void BluetoothLowEnergyConnectionFinder::OnDiscoverySessionStarted(
    scoped_ptr<device::BluetoothDiscoverySession> discovery_session) {
  VLOG(1) << "Discovery session started";
  discovery_session_ = discovery_session.Pass();
}

void BluetoothLowEnergyConnectionFinder::OnStartDiscoverySessionError() {
  VLOG(1) << "Error starting discovery session";
}

void BluetoothLowEnergyConnectionFinder::StartDiscoverySession() {
  DCHECK(adapter_);
  if (discovery_session_ && discovery_session_->IsActive()) {
    VLOG(1) << "Discovery session already active";
    return;
  }

  // Discover only low energy (LE) devices with strong enough signal.
  scoped_ptr<BluetoothDiscoveryFilter> filter(new BluetoothDiscoveryFilter(
      BluetoothDiscoveryFilter::Transport::TRANSPORT_LE));
  filter->SetRSSI(kMinDiscoveryRSSI);

  adapter_->StartDiscoverySessionWithFilter(
      filter.Pass(),
      base::Bind(&BluetoothLowEnergyConnectionFinder::OnDiscoverySessionStarted,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(
          &BluetoothLowEnergyConnectionFinder::OnStartDiscoverySessionError,
          weak_ptr_factory_.GetWeakPtr()));
}

void BluetoothLowEnergyConnectionFinder::OnDiscoverySessionStopped() {
  VLOG(1) << "Discovery session stopped";
  discovery_session_.reset();
}

void BluetoothLowEnergyConnectionFinder::OnStopDiscoverySessionError() {
  VLOG(1) << "Error stopping discovery session";
}

void BluetoothLowEnergyConnectionFinder::StopDiscoverySession() {
  VLOG(1) << "Stopping discovery sesison";

  if (!adapter_) {
    VLOG(1) << "Adapter not initialized";
    return;
  }
  if (!discovery_session_ || !discovery_session_->IsActive()) {
    VLOG(1) << "No Active discovery session";
    return;
  }

  discovery_session_->Stop(
      base::Bind(&BluetoothLowEnergyConnectionFinder::OnDiscoverySessionStopped,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(
          &BluetoothLowEnergyConnectionFinder::OnStopDiscoverySessionError,
          weak_ptr_factory_.GetWeakPtr()));
}

bool BluetoothLowEnergyConnectionFinder::HasService(
    BluetoothDevice* remote_device) {
  if (remote_device) {
    VLOG(1) << "Device " << remote_device->GetAddress() << " has "
            << remote_device->GetUUIDs().size() << " services.";
    std::vector<device::BluetoothUUID> uuids = remote_device->GetUUIDs();
    for (const auto& service_uuid : uuids) {
      if (remote_service_uuid_ == service_uuid) {
        return true;
      }
    }
  }
  return false;
}

void BluetoothLowEnergyConnectionFinder::OnCreateGattConnectionError(
    std::string device_address,
    BluetoothDevice::ConnectErrorCode error_code) {
  VLOG(1) << "Error creating connection to device " << device_address
          << " : error code = " << error_code;
}

void BluetoothLowEnergyConnectionFinder::OnGattConnectionCreated(
    scoped_ptr<BluetoothGattConnection> gatt_connection) {
  if (connected_) {
    CloseGattConnection(gatt_connection.Pass());
    return;
  }

  VLOG(1) << "GATT connection created";
  connected_ = true;
  pending_connections_.clear();

  gatt_connection_ = gatt_connection.Pass();

  // This is a workaround for crbug.com/498850. Currently, trying to write/read
  // characteristics immediatelly after the GATT connection was established
  // fails with the very informative GATT_ERROR_FAILED.
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&BluetoothLowEnergyConnectionFinder::CompleteConnection,
                 weak_ptr_factory_.GetWeakPtr()),
      delay_after_gatt_connection_);

  StopDiscoverySession();
}

void BluetoothLowEnergyConnectionFinder::CompleteConnection() {
  connection_ = CreateConnection(gatt_connection_.Pass());
  connection_->AddObserver(this);
  connection_->Connect();
}

void BluetoothLowEnergyConnectionFinder::CreateGattConnection(
    device::BluetoothDevice* remote_device) {
  VLOG(1) << "SmartLock service found ("
          << remote_service_uuid_.canonical_value() << ")\n"
          << "device = " << remote_device->GetAddress()
          << ", name = " << remote_device->GetName();
  remote_device->CreateGattConnection(
      base::Bind(&BluetoothLowEnergyConnectionFinder::OnGattConnectionCreated,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(
          &BluetoothLowEnergyConnectionFinder::OnCreateGattConnectionError,
          weak_ptr_factory_.GetWeakPtr(), remote_device->GetAddress()));
}

void BluetoothLowEnergyConnectionFinder::CloseGattConnection(
    scoped_ptr<device::BluetoothGattConnection> gatt_connection) {
  DCHECK(gatt_connection);
  std::string device_address = gatt_connection->GetDeviceAddress();
  gatt_connection.reset();
  BluetoothDevice* device = adapter_->GetDevice(device_address);
  if (device) {
    VLOG(1) << "Disconnect from device " << device->GetAddress();
    device->Disconnect(base::Bind(&base::DoNothing),
                       base::Bind(&base::DoNothing));
  }
}

scoped_ptr<Connection> BluetoothLowEnergyConnectionFinder::CreateConnection(
    scoped_ptr<BluetoothGattConnection> gatt_connection) {
  remote_device_.bluetooth_address = gatt_connection->GetDeviceAddress();

  return make_scoped_ptr(new BluetoothLowEnergyConnection(
      remote_device_, adapter_, remote_service_uuid_, to_peripheral_char_uuid_,
      from_peripheral_char_uuid_, gatt_connection.Pass(),
      max_number_of_tries_));
}

void BluetoothLowEnergyConnectionFinder::OnConnectionStatusChanged(
    Connection* connection,
    Connection::Status old_status,
    Connection::Status new_status) {
  DCHECK_EQ(connection, connection_.get());

  if (!connection_callback_.is_null() && connection_->IsConnected()) {
    adapter_->RemoveObserver(this);
    connection_->RemoveObserver(this);

    // Note: any observer of |connection_| added in |connection_callback_| will
    // also receive this |OnConnectionStatusChanged| notification (IN_PROGRESS
    // -> CONNECTED).
    connection_callback_.Run(connection_.Pass());
    connection_callback_.Reset();
  } else if (old_status == Connection::IN_PROGRESS) {
    VLOG(1) << "Connection failed. Retrying.";
    RestartDiscoverySessionWhenReady();
  }
}

void BluetoothLowEnergyConnectionFinder::RestartDiscoverySessionWhenReady() {
  // It's not possible to simply use
  // |adapter_->GetDevice(GetRemoteDeviceAddress())| to find the device with MAC
  // address |GetRemoteDeviceAddress()|. For paired devices,
  // BluetoothAdapter::GetDevice(XXX) searches for the temporary MAC address
  // XXX, whereas |remote_device_.bluetooth_address| is the real MAC address.
  // This is a bug in the way device::BluetoothAdapter is storing the devices
  // (see crbug.com/497841).
  BluetoothDevice* device = nullptr;
  std::vector<BluetoothDevice*> devices = adapter_->GetDevices();
  for (const auto& dev : devices) {
    if (dev->GetAddress() == remote_device_.bluetooth_address)
      device = dev;
  }

  // To restart scanning for devices, it's necessary to ensure that:
  // (i) the GATT connection to |remove_device_| is closed;
  // (ii) there is no pending call to
  // |device::BluetoothDiscoverySession::Stop()|.
  // The second condition is satisfied when |OnDiscoveryStopped| is called and
  // |discovery_session_| is reset.
  if ((!device || !device->IsConnected()) && !discovery_session_) {
    DCHECK(!gatt_connection_);
    connection_.reset();
    connected_ = false;
    StartDiscoverySession();
  } else {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&BluetoothLowEnergyConnectionFinder::
                                  RestartDiscoverySessionWhenReady,
                              weak_ptr_factory_.GetWeakPtr()));
  }
}

void BluetoothLowEnergyConnectionFinder::SetDelayForTesting(
    base::TimeDelta delay) {
  delay_after_gatt_connection_ = delay;
}

}  // namespace proximity_auth
