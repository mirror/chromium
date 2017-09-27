// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/u2f_ble_connection.h"

#include "base/bind.h"
#include "base/logging.h"

#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_gatt_connection.h"
#include "device/bluetooth/bluetooth_gatt_notify_session.h"
#include "device/bluetooth/bluetooth_remote_gatt_characteristic.h"
#include "device/bluetooth/bluetooth_uuid.h"
#include "device/u2f/u2f_ble_uuids.h"

#include <utility>

namespace device {

U2fBleConnection::U2fBleConnection(std::string device_address,
                                   ConnectCallback connect_callback,
                                   ReadCallback read_callback)
    : address_(std::move(device_address)),
      connect_callback_(std::move(connect_callback)),
      read_callback_(std::move(read_callback)),
      weak_factory_(this) {
  auto& factory = BluetoothAdapterFactory::Get();
  factory.GetAdapter(
      base::Bind(&This::GetAdapterCallback, weak_factory_.GetWeakPtr()));
}

U2fBleConnection::~U2fBleConnection() {
  adapter_->RemoveObserver(this);
}

void U2fBleConnection::ReadControlPointLength(
    ControlPointLengthCallback callback) {
  auto read_callback = [](ControlPointLengthCallback callback,
                          const std::vector<uint8_t>& value) {
    if (value.size() != 2) {
      LOG(ERROR) << "Wrong Control Point Length.";
      callback.Run(0);
      return;
    }
    uint16_t length = (static_cast<uint16_t>(value[0]) << 8) + value[1];
    VLOG(2) << "Control Point Length: " << length;
    callback.Run(length);
  };

  control_point_length_->ReadRemoteCharacteristic(
      base::Bind(read_callback, callback),
      base::Bind(
          [](ControlPointLengthCallback callback,
             BluetoothGattService::GattErrorCode code) {
            LOG(ERROR) << "GATT error " << static_cast<int>(code);
            callback.Run(0);
          },
          callback));
}

void U2fBleConnection::Write(const std::vector<uint8_t>& data,
                             WriteCallback callback) {
  control_point_->WriteRemoteCharacteristic(
      data,
      base::Bind([](WriteCallback callback) { callback.Run(true); }, callback),
      base::Bind(
          [](WriteCallback callback, BluetoothGattService::GattErrorCode code) {
            LOG(ERROR) << "GATT error " << static_cast<int>(code);
            callback.Run(false);
          },
          callback));
}

void U2fBleConnection::GetAdapterCallback(
    scoped_refptr<BluetoothAdapter> adapter) {
  adapter_ = std::move(adapter);
  DCHECK(!!adapter_);
  VLOG(2) << "Got adapter " << adapter_->GetAddress();

  adapter_->AddObserver(this);
  if (adapter_->IsPowered()) {
    OnSetPowered();
  } else {
    adapter_->SetPowered(
        true, base::Bind(&This::OnSetPowered, weak_factory_.GetWeakPtr()),
        base::Bind(
            [](ConnectCallback callback) {
              LOG(ERROR) << "Failed to power on the device.";
              callback.Run(false);
            },
            connect_callback_));
  }
}

class Delegate : public BluetoothDevice::PairingDelegate {
 public:
  ~Delegate() override = default;

  void RequestPinCode(BluetoothDevice* device) override {
    static constexpr const char* pin = "868970";  // FIXME
    VLOG(2) << "Requested PIN. Entering " << pin;
    device->SetPinCode(pin);
  }

  void RequestPasskey(BluetoothDevice* device) override {
    uint32_t passkey = 868970;
    // uint32_t passkey = 915405;
    VLOG(2) << "Requested PassKey. Entering " << passkey;
    device->SetPasskey(passkey);
    device->ConfirmPairing();
  }

  void DisplayPinCode(BluetoothDevice* device,
                      const std::string& pincode) override {
    VLOG(2) << "Display PIN: " << pincode;
  }

  void DisplayPasskey(BluetoothDevice* device, uint32_t passkey) override {
    VLOG(2) << "Display PassKey: " << passkey;
  }

  void KeysEntered(BluetoothDevice* device, uint32_t entered) override {
    VLOG(2) << "Keys entered: " << entered;
  }

  void ConfirmPasskey(BluetoothDevice* device, uint32_t passkey) override {
    VLOG(2) << "Confirm PassKey: " << passkey;
  }

  void AuthorizePairing(BluetoothDevice* device) override {
    VLOG(2) << "Authorize pairing.";
  }
};

void U2fBleConnection::OnSetPowered() {
  VLOG(2) << "Adapter " << adapter_->GetAddress() << " is powered on.";

  device_ = adapter_->GetDevice(address_);
  if (!device_) {
    LOG(ERROR) << "Device " << address_ << " not found.";
    connect_callback_.Run(false);
    return;
  }

  VLOG(2) << "Found device " << device_->GetAddress() << " ("
          << device_->GetName().value() << "); flags: " << device_->IsPaired()
          << device_->IsPairable() << device_->IsConnected()
          << device_->IsConnecting() << device_->IsConnectable()
          << device_->IsGattConnected()
          << device_->IsGattServicesDiscoveryComplete()
          << device_->IsGattConnected()
          << "; uuids: " << device_->GetUUIDs().size();

  std::unique_ptr<Delegate> delegate(new Delegate);
  device_->Connect(
      delegate.release(),
      base::Bind(&This::OnConnected, weak_factory_.GetWeakPtr()),
      base::Bind(
          [](ConnectCallback callback, BluetoothDevice::ConnectErrorCode code) {
            LOG(ERROR) << "Failed to connect: " << static_cast<int>(code);
            callback.Run(false);
          },
          connect_callback_));
}

void U2fBleConnection::OnConnected() {
  VLOG(2) << "Connected";
  device_->CreateGattConnection(
      base::Bind(&This::OnGattConnected, weak_factory_.GetWeakPtr()),
      base::Bind(
          [](ConnectCallback callback, BluetoothDevice::ConnectErrorCode code) {
            LOG(ERROR) << "Failed to create GATT connection: "
                       << static_cast<int>(code);
            callback.Run(false);
          },
          connect_callback_));
}

void U2fBleConnection::OnGattConnected(
    std::unique_ptr<BluetoothGattConnection> connection) {
  VLOG(2) << "Connected to GATT on " << device_->GetAddress();
  gatt_connection_ = std::move(connection);

  if (device_->IsGattServicesDiscoveryComplete()) {
    ConnectToU2fService();
  }
}

void U2fBleConnection::GattServicesDiscovered(BluetoothAdapter* adapter,
                                              BluetoothDevice* device) {
  if (device != device_)
    return;
  VLOG(2) << "GATT services discovered on " << device_->GetAddress();

  if (gatt_connection_) {
    ConnectToU2fService();
  }
}

void U2fBleConnection::ConnectToU2fService() {
  DCHECK(device_->IsGattServicesDiscoveryComplete());
  VLOG(2) << "Connecting to U2F service on " << device_->GetAddress();

  auto services =
      device_->GetPrimaryServicesByUUID(BluetoothUUID(U2F_SERVICE_UUID));
  if (services.empty()) {
    LOG(ERROR) << "U2F service not found on " << device_->GetAddress();
    connect_callback_.Run(false);
    return;
  }
  if (services.size() > 1) {
    VLOG(2) << "Found multiple U2F services (" << services.size() << ") on "
            << device_->GetAddress() << ", using the first one.";
  }

  auto characteristics = services[0]->GetCharacteristics();
  for (auto* characteristic : characteristics) {
    const std::string uuid = characteristic->GetUUID().canonical_value();
    if (uuid == U2F_CONTROL_POINT_LENGTH_UUID) {
      control_point_length_ = characteristic;
    } else if (uuid == U2F_CONTROL_POINT_UUID) {
      control_point_ = characteristic;
    } else if (uuid == U2F_STATUS_UUID) {
      status_ = characteristic;
    }
  }

  if (!control_point_length_ || !control_point_ || !status_) {
    LOG(ERROR) << "U2F characteristics missing.";
    connect_callback_.Run(false);
    return;
  }

  status_->StartNotifySession(
      base::Bind(&This::NotifySessionCallback, weak_factory_.GetWeakPtr()),
      base::Bind(
          [](ConnectCallback callback,
             BluetoothGattService::GattErrorCode code) {
            LOG(ERROR) << "GATT error " << static_cast<int>(code);
            callback.Run(false);
          },
          connect_callback_));
}

void U2fBleConnection::NotifySessionCallback(
    std::unique_ptr<BluetoothGattNotifySession> session) {
  status_notification_session_ = std::move(session);
  VLOG(2) << "Created notification session. Connection established.";
  connect_callback_.Run(true);
}

// BluetoothAdapter::Observer:
void U2fBleConnection::GattCharacteristicValueChanged(
    BluetoothAdapter* adapter,
    BluetoothRemoteGattCharacteristic* characteristic,
    const std::vector<uint8_t>& value) {
  if (adapter != adapter_ || characteristic != status_)
    return;
  VLOG(2) << "Status characteristic value changed.";
  read_callback_.Run(std::move(value));
}

}  // namespace device
