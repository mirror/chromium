// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/u2f_ble_discovery.h"

#include "base/bind.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_common.h"
#include "device/bluetooth/bluetooth_discovery_filter.h"
#include "device/bluetooth/bluetooth_discovery_session.h"
#include "device/bluetooth/bluetooth_uuid.h"
#include "device/u2f/u2f_ble_uuids.h"

#include "base/memory/ptr_util.h"
#include "base/stl_util.h"

namespace device {

U2fBleDiscovery::U2fBleDiscovery() : weak_factory_(this) {}

U2fBleDiscovery::~U2fBleDiscovery() {
  adapter_->RemoveObserver(this);
}

void U2fBleDiscovery::Start() {
  auto& factory = BluetoothAdapterFactory::Get();
  factory.GetAdapter(base::Bind(&U2fBleDiscovery::GetAdapterCallback,
                                weak_factory_.GetWeakPtr()));
}

void U2fBleDiscovery::Stop() {
  // discovery_session_->Stop(base::Closure(), base::Closure());
  discovery_session_.reset(nullptr);
}

void U2fBleDiscovery::GetAdapterCallback(
    scoped_refptr<BluetoothAdapter> adapter) {
  adapter_ = std::move(adapter);
  DCHECK(!!adapter_);
  VLOG(2) << "Got adapter " << adapter_->GetAddress();

  adapter_->AddObserver(this);
  if (adapter_->IsPowered()) {
    OnSetPowered();
  } else {
    adapter_->SetPowered(
        true,
        base::Bind(&U2fBleDiscovery::OnSetPowered, weak_factory_.GetWeakPtr()),
        base::Bind([]() { LOG(ERROR) << "Failed to power on the device."; }));
  }
}

void U2fBleDiscovery::OnSetPowered() {
  VLOG(2) << "Adapter " << adapter_->GetAddress() << " is powered on.";

  BluetoothUUID service_uuid(U2F_SERVICE_UUID);

  auto devices = adapter_->GetDevices();
  for (BluetoothDevice* device : devices) {
    if (base::ContainsKey(device->GetUUIDs(), service_uuid)) {
      VLOG(2) << "Good device: " << device->GetAddress();
      devices_.insert(device->GetAddress());
    }
  }

  auto filter = base::MakeUnique<BluetoothDiscoveryFilter>(
      BluetoothTransport::BLUETOOTH_TRANSPORT_LE);
  filter->AddUUID(service_uuid);

  adapter_->StartDiscoverySessionWithFilter(
      std::move(filter),
      base::Bind(&U2fBleDiscovery::DiscoverySessionStarted,
                 weak_factory_.GetWeakPtr()),
      base::Bind([]() { LOG(ERROR) << "Discovery session not started."; }));
}

void U2fBleDiscovery::DiscoverySessionStarted(
    std::unique_ptr<BluetoothDiscoverySession> session) {
  discovery_session_ = std::move(session);
  VLOG(2) << "Discovery started.";
}

void U2fBleDiscovery::DeviceAdded(BluetoothAdapter* adapter,
                                  BluetoothDevice* device) {
  BluetoothUUID service_uuid(U2F_SERVICE_UUID);
  if (base::ContainsKey(device->GetUUIDs(), service_uuid)) {
    VLOG(2) << "Good device: " << device->GetAddress();
    devices_.insert(device->GetAddress());
  }
}

void U2fBleDiscovery::DeviceRemoved(BluetoothAdapter* adapter,
                                    BluetoothDevice* device) {
  if (devices_.erase(device->GetAddress())) {
    VLOG(2) << "Good device removed: " << device->GetAddress();
  }
}

}  // namespace device
