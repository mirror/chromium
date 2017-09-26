// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/u2f_ble_discovery.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_common.h"
#include "device/bluetooth/bluetooth_discovery_filter.h"
#include "device/bluetooth/bluetooth_discovery_session.h"
#include "device/bluetooth/bluetooth_uuid.h"
#include "device/u2f/u2f_ble_uuids.h"

namespace device {

U2fBleDiscovery::U2fBleDiscovery() : weak_factory_(this) {}

U2fBleDiscovery::~U2fBleDiscovery() {
  adapter_->RemoveObserver(this);
}

void U2fBleDiscovery::Start(DiscoveryStartedCallback started,
                            DeviceStatusCallback callback) {
  DCHECK(callback_.is_null());
  callback_ = std::move(callback);

  auto& factory = BluetoothAdapterFactory::Get();
  factory.GetAdapter(base::Bind(&U2fBleDiscovery::GetAdapterCallback,
                                weak_factory_.GetWeakPtr(),
                                base::Passed(&started)));
}

void U2fBleDiscovery::Stop() {
  // discovery_session_->Stop(base::Closure(), base::Closure());
  discovery_session_.reset(nullptr);
  callback_.Reset();
}

void U2fBleDiscovery::GetAdapterCallback(
    DiscoveryStartedCallback started,
    scoped_refptr<BluetoothAdapter> adapter) {
  adapter_ = std::move(adapter);
  DCHECK(!!adapter_);
  VLOG(2) << "Got adapter " << adapter_->GetAddress();

  adapter_->AddObserver(this);
  if (adapter_->IsPowered()) {
    OnSetPowered(std::move(started));
  } else {
    auto started_copy = started;
    adapter_->SetPowered(
        true,
        base::Bind(&U2fBleDiscovery::OnSetPowered, weak_factory_.GetWeakPtr(),
                   base::Passed(&started)),
        base::Bind(
            [](DiscoveryStartedCallback started) {
              LOG(ERROR) << "Failed to power on the adapter.";
              started.Run(false);
            },
            base::Passed(&started_copy)));
  }
}

void U2fBleDiscovery::OnSetPowered(DiscoveryStartedCallback started) {
  VLOG(2) << "Adapter " << adapter_->GetAddress() << " is powered on.";

  BluetoothUUID service_uuid(U2F_SERVICE_UUID);

  auto devices = adapter_->GetDevices();
  for (BluetoothDevice* device : devices) {
    if (base::ContainsKey(device->GetUUIDs(), service_uuid)) {
      const std::string address = device->GetAddress();
      VLOG(2) << "U2F BLE device: " << address;

      devices_.insert(address);
      callback_.Run(address, DeviceStatus::KNOWN);
    }
  }

  auto filter = base::MakeUnique<BluetoothDiscoveryFilter>(
      BluetoothTransport::BLUETOOTH_TRANSPORT_LE);
  filter->AddUUID(service_uuid);

  auto started_copy = started;
  adapter_->StartDiscoverySessionWithFilter(
      std::move(filter),
      base::Bind(&U2fBleDiscovery::DiscoverySessionStarted,
                 weak_factory_.GetWeakPtr(), base::Passed(&started)),
      base::Bind(
          [](DiscoveryStartedCallback started) {
            LOG(ERROR) << "Discovery session not started.";
            started.Run(false);
          },
          base::Passed(&started_copy)));
}

void U2fBleDiscovery::DiscoverySessionStarted(
    DiscoveryStartedCallback started,
    std::unique_ptr<BluetoothDiscoverySession> session) {
  discovery_session_ = std::move(session);
  VLOG(2) << "Discovery session started.";
  started.Run(true);
}

void U2fBleDiscovery::DeviceAdded(BluetoothAdapter* adapter,
                                  BluetoothDevice* device) {
  BluetoothUUID service_uuid(U2F_SERVICE_UUID);
  if (base::ContainsKey(device->GetUUIDs(), service_uuid)) {
    const std::string address = device->GetAddress();
    VLOG(2) << "Discovered U2F BLE device: " << address;
    devices_.insert(address);
    callback_.Run(address, DeviceStatus::ADDED);
  }
}

void U2fBleDiscovery::DeviceRemoved(BluetoothAdapter* adapter,
                                    BluetoothDevice* device) {
  if (devices_.erase(device->GetAddress())) {
    const std::string address = device->GetAddress();
    VLOG(2) << "U2F BLE device removed: " << device->GetAddress();
    callback_.Run(address, DeviceStatus::REMOVED);
  }
}

}  // namespace device
