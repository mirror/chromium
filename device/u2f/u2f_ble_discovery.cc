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
#include "device/u2f/u2f_device.h"

namespace device {

namespace {

class U2fFakeBleDevice : public U2fDevice {
 public:
  static std::string GetId(std::string address) {
    return "ble:" + std::move(address);
  }

  U2fFakeBleDevice(std::string address)
      : id_(GetId(std::move(address))), weak_factory_(this) {}

  ~U2fFakeBleDevice() override = default;

  void TryWink(const WinkCallback& callback) override {}
  std::string GetId() const override { return id_; }

 protected:
  void DeviceTransact(std::unique_ptr<U2fApduCommand> command,
                      const DeviceCallback& callback) override {
    callback.Run(false, nullptr);
  }

  base::WeakPtr<U2fDevice> GetWeakPtr() override {
    return weak_factory_.GetWeakPtr();
  }

 private:
  std::string id_;

  base::WeakPtrFactory<U2fFakeBleDevice> weak_factory_;
};

}  // namespace

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
  adapter_->RemoveObserver(this);

  discovery_session_->Stop(
      base::Bind(&U2fBleDiscovery::OnStopped, weak_factory_.GetWeakPtr(), true),
      base::Bind(&U2fBleDiscovery::OnStopped, weak_factory_.GetWeakPtr(),
                 false));
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
        base::Bind(
            [](base::WeakPtr<Delegate> delegate) {
              LOG(ERROR) << "Failed to power on the adapter.";
              delegate->OnStarted(false);
            },
            delegate_));
  }
}

void U2fBleDiscovery::OnSetPowered() {
  VLOG(2) << "Adapter " << adapter_->GetAddress() << " is powered on.";

  BluetoothUUID service_uuid(U2F_SERVICE_UUID);

  auto devices = adapter_->GetDevices();
  for (BluetoothDevice* device : devices) {
    if (base::ContainsKey(device->GetUUIDs(), service_uuid)) {
      const std::string address = device->GetAddress();
      VLOG(2) << "U2F BLE device: " << address;
      devices_.insert(address);

      std::unique_ptr<U2fDevice> device(new U2fFakeBleDevice(address));
      delegate_->OnDeviceAdded(std::move(device));
    }
  }

  auto filter = base::MakeUnique<BluetoothDiscoveryFilter>(
      BluetoothTransport::BLUETOOTH_TRANSPORT_LE);
  filter->AddUUID(service_uuid);

  adapter_->StartDiscoverySessionWithFilter(
      std::move(filter),
      base::Bind(&U2fBleDiscovery::DiscoverySessionStarted,
                 weak_factory_.GetWeakPtr()),
      base::Bind(
          [](base::WeakPtr<Delegate> delegate) {
            LOG(ERROR) << "Discovery session not started.";
            delegate->OnStarted(false);
          },
          delegate_));
}

void U2fBleDiscovery::DiscoverySessionStarted(
    std::unique_ptr<BluetoothDiscoverySession> session) {
  discovery_session_ = std::move(session);
  VLOG(2) << "Discovery session started.";
  delegate_->OnStarted(true);
}

void U2fBleDiscovery::DeviceAdded(BluetoothAdapter* adapter,
                                  BluetoothDevice* device) {
  BluetoothUUID service_uuid(U2F_SERVICE_UUID);
  if (base::ContainsKey(device->GetUUIDs(), service_uuid)) {
    const std::string address = device->GetAddress();
    VLOG(2) << "Discovered U2F BLE device: " << address;
    devices_.insert(address);

    std::unique_ptr<U2fDevice> device(new U2fFakeBleDevice(address));
    delegate_->OnDeviceAdded(std::move(device));
  }
}

void U2fBleDiscovery::DeviceRemoved(BluetoothAdapter* adapter,
                                    BluetoothDevice* device) {
  if (devices_.erase(device->GetAddress())) {
    const std::string address = device->GetAddress();
    VLOG(2) << "U2F BLE device removed: " << device->GetAddress();
    delegate_->OnDeviceRemoved(U2fFakeBleDevice::GetId(address));
  }
}

void U2fBleDiscovery::OnStopped(bool success) {
  discovery_session_.reset(nullptr);
  delegate_->OnStopped(success);
}

}  // namespace device
