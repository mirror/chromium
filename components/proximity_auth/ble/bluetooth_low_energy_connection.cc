// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/proximity_auth/ble/bluetooth_low_energy_connection.h"

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "components/proximity_auth/ble/bluetooth_low_energy_characteristics_finder.h"
#include "components/proximity_auth/ble/fake_wire_message.h"
#include "components/proximity_auth/connection_finder.h"
#include "components/proximity_auth/wire_message.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_gatt_characteristic.h"
#include "device/bluetooth/bluetooth_gatt_connection.h"
#include "device/bluetooth/bluetooth_gatt_notify_session.h"
#include "device/bluetooth/bluetooth_uuid.h"

using device::BluetoothAdapter;
using device::BluetoothDevice;
using device::BluetoothGattConnection;
using device::BluetoothGattService;
using device::BluetoothGattCharacteristic;
using device::BluetoothGattNotifySession;
using device::BluetoothUUID;

namespace proximity_auth {
namespace {
const int kFirstByteZero = 0;
}  // namespace

BluetoothLowEnergyConnection::BluetoothLowEnergyConnection(
    const RemoteDevice& device,
    scoped_refptr<device::BluetoothAdapter> adapter,
    const BluetoothUUID remote_service_uuid,
    const BluetoothUUID to_peripheral_char_uuid,
    const BluetoothUUID from_peripheral_char_uuid,
    scoped_ptr<BluetoothGattConnection> gatt_connection,
    int max_number_of_write_attempts)
    : Connection(device),
      adapter_(adapter),
      remote_service_({remote_service_uuid, ""}),
      to_peripheral_char_({to_peripheral_char_uuid, ""}),
      from_peripheral_char_({from_peripheral_char_uuid, ""}),
      connection_(gatt_connection.Pass()),
      sub_status_(SubStatus::DISCONNECTED),
      receiving_bytes_(false),
      write_remote_characteristic_pending_(false),
      max_number_of_write_attempts_(max_number_of_write_attempts),
      weak_ptr_factory_(this) {
  DCHECK(adapter_);
  DCHECK(adapter_->IsInitialized());

  start_time_ = base::TimeTicks::Now();
  adapter_->AddObserver(this);
}

BluetoothLowEnergyConnection::~BluetoothLowEnergyConnection() {
  Disconnect();
  if (adapter_) {
    adapter_->RemoveObserver(this);
    adapter_ = NULL;
  }
}

void BluetoothLowEnergyConnection::Connect() {
  if (connection_ && connection_->IsConnected()) {
    OnGattConnectionCreated(connection_.Pass());
    return;
  }

  BluetoothDevice* remote_device = GetRemoteDevice();
  if (remote_device) {
    SetSubStatus(SubStatus::WAITING_GATT_CONNECTION);
    remote_device->CreateGattConnection(
        base::Bind(&BluetoothLowEnergyConnection::OnGattConnectionCreated,
                   weak_ptr_factory_.GetWeakPtr()),
        base::Bind(&BluetoothLowEnergyConnection::OnCreateGattConnectionError,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

// This actually forgets the remote BLE device. This is safe as long as we only
// connect to BLE devices advertising the SmartLock service (assuming this
// device has no other connection).
void BluetoothLowEnergyConnection::Disconnect() {
  if (sub_status_ != SubStatus::DISCONNECTED) {
    ClearWriteRequestsQueue();
    StopNotifySession();
    SetSubStatus(SubStatus::DISCONNECTED);
    if (connection_) {
      connection_.reset();
      BluetoothDevice* device = GetRemoteDevice();
      if (device) {
        VLOG(1) << "Forget device " << device->GetAddress();
        device->Forget(base::Bind(&base::DoNothing));
      }
    }
  }
}

void BluetoothLowEnergyConnection::SetSubStatus(SubStatus new_sub_status) {
  sub_status_ = new_sub_status;

  // Sets the status of parent class proximity_auth::Connection accordingly.
  if (new_sub_status == SubStatus::CONNECTED) {
    SetStatus(CONNECTED);
  } else if (new_sub_status == SubStatus::DISCONNECTED) {
    SetStatus(DISCONNECTED);
  } else {
    SetStatus(IN_PROGRESS);
  }
}

// TODO(sacomoto): Implement a sender with full support for messages larger than
// a single characteristic value.
void BluetoothLowEnergyConnection::SendMessageImpl(
    scoped_ptr<WireMessage> message) {
  VLOG(1) << "Sending message " << message->Serialize();

  std::string serialized_msg = message->Serialize();

  WriteRequest write_request = BuildWriteRequest(
      ToByteVector(static_cast<uint32>(ControlSignal::kSendSignal)),
      ToByteVector(static_cast<uint32>(serialized_msg.size())), false);
  WriteRemoteCharacteristic(write_request);

  write_request = BuildWriteRequest(
      std::vector<uint8>{static_cast<uint8>(kFirstByteZero)},
      std::vector<uint8>(serialized_msg.begin(), serialized_msg.end()), true);
  WriteRemoteCharacteristic(write_request);
}

void BluetoothLowEnergyConnection::DeviceRemoved(BluetoothAdapter* adapter,
                                                 BluetoothDevice* device) {
  if (device && device->GetAddress() == GetRemoteDeviceAddress()) {
    VLOG(1) << "Device removed " << GetRemoteDeviceAddress();
    Disconnect();
  }
}

// TODO(sacomoto): Implement a receiver with full support for messages larger
// than a single characteristic value.
void BluetoothLowEnergyConnection::GattCharacteristicValueChanged(
    BluetoothAdapter* adapter,
    BluetoothGattCharacteristic* characteristic,
    const std::vector<uint8>& value) {
  DCHECK_EQ(adapter, adapter_.get());

  VLOG(1) << "Characteristic value changed: "
          << characteristic->GetUUID().canonical_value();

  if (characteristic->GetIdentifier() == from_peripheral_char_.id) {
    if (receiving_bytes_) {
      // Ignoring the first byte, as it contains a deprecated signal.
      const std::string bytes(value.begin() + 1, value.end());
      OnBytesReceived(bytes);
      receiving_bytes_ = false;
      return;
    }

    if (value.size() < 4) {
      VLOG(1) << "Incoming data corrupted, no signal found.";
      return;
    }

    const ControlSignal signal = static_cast<ControlSignal>(ToUint32(value));
    switch (signal) {
      case ControlSignal::kInvitationResponseSignal:
        if (sub_status() == SubStatus::WAITING_RESPONSE_SIGNAL)
          CompleteConnection();
        break;
      case ControlSignal::kInviteToConnectSignal:
        break;
      case ControlSignal::kSendSignal:
        receiving_bytes_ = true;
        break;
      case ControlSignal::kDisconnectSignal:
        Disconnect();
        break;
    }
  }
}

BluetoothLowEnergyConnection::WriteRequest::WriteRequest(
    const std::vector<uint8>& val,
    bool flag)
    : value(val),
      is_last_write_for_wire_message(flag),
      number_of_failed_attempts(0) {
}

BluetoothLowEnergyConnection::WriteRequest::~WriteRequest() {
}

scoped_ptr<WireMessage> BluetoothLowEnergyConnection::DeserializeWireMessage(
    bool* is_incomplete_message) {
  return FakeWireMessage::Deserialize(received_bytes(), is_incomplete_message);
}

void BluetoothLowEnergyConnection::CompleteConnection() {
  VLOG(1) << "Connection completed\n"
          << "Time elapsed: " << base::TimeTicks::Now() - start_time_;
  SetSubStatus(SubStatus::CONNECTED);
}

void BluetoothLowEnergyConnection::OnCreateGattConnectionError(
    device::BluetoothDevice::ConnectErrorCode error_code) {
  VLOG(1) << "Error creating GATT connection to "
          << remote_device().bluetooth_address << "error code: " << error_code;
  Disconnect();
}

void BluetoothLowEnergyConnection::OnGattConnectionCreated(
    scoped_ptr<device::BluetoothGattConnection> gatt_connection) {
  connection_ = gatt_connection.Pass();
  SetSubStatus(SubStatus::WAITING_CHARACTERISTICS);
  characteristic_finder_.reset(CreateCharacteristicsFinder(
      base::Bind(&BluetoothLowEnergyConnection::OnCharacteristicsFound,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&BluetoothLowEnergyConnection::OnCharacteristicsFinderError,
                 weak_ptr_factory_.GetWeakPtr())));
}

BluetoothLowEnergyCharacteristicsFinder*
BluetoothLowEnergyConnection::CreateCharacteristicsFinder(
    const BluetoothLowEnergyCharacteristicsFinder::SuccessCallback&
        success_callback,
    const BluetoothLowEnergyCharacteristicsFinder::ErrorCallback&
        error_callback) {
  return new BluetoothLowEnergyCharacteristicsFinder(
      adapter_, GetRemoteDevice(), remote_service_, to_peripheral_char_,
      from_peripheral_char_, success_callback, error_callback);
}

void BluetoothLowEnergyConnection::OnCharacteristicsFound(
    const RemoteAttribute& service,
    const RemoteAttribute& to_peripheral_char,
    const RemoteAttribute& from_peripheral_char) {
  remote_service_ = service;
  to_peripheral_char_ = to_peripheral_char;
  from_peripheral_char_ = from_peripheral_char;

  SetSubStatus(SubStatus::CHARACTERISTICS_FOUND);
  StartNotifySession();
}

void BluetoothLowEnergyConnection::OnCharacteristicsFinderError(
    const RemoteAttribute& to_peripheral_char,
    const RemoteAttribute& from_peripheral_char) {
  VLOG(1) << "Connection error, missing characteristics for SmartLock "
             "service.\n" << (to_peripheral_char.id.empty()
                                  ? to_peripheral_char.uuid.canonical_value()
                                  : "")
          << (from_peripheral_char.id.empty()
                  ? ", " + from_peripheral_char.uuid.canonical_value()
                  : "") << " not found.";

  Disconnect();
}

void BluetoothLowEnergyConnection::StartNotifySession() {
  if (sub_status() == SubStatus::CHARACTERISTICS_FOUND) {
    BluetoothGattCharacteristic* characteristic =
        GetGattCharacteristic(from_peripheral_char_.id);
    DCHECK(characteristic);

    SetSubStatus(SubStatus::WAITING_NOTIFY_SESSION);
    characteristic->StartNotifySession(
        base::Bind(&BluetoothLowEnergyConnection::OnNotifySessionStarted,
                   weak_ptr_factory_.GetWeakPtr()),
        base::Bind(&BluetoothLowEnergyConnection::OnNotifySessionError,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

void BluetoothLowEnergyConnection::OnNotifySessionError(
    BluetoothGattService::GattErrorCode error) {
  VLOG(1) << "Error starting notification session: " << error;
  Disconnect();
}

void BluetoothLowEnergyConnection::OnNotifySessionStarted(
    scoped_ptr<BluetoothGattNotifySession> notify_session) {
  VLOG(1) << "Notification session started "
          << notify_session->GetCharacteristicIdentifier();

  SetSubStatus(SubStatus::NOTIFY_SESSION_READY);
  notify_session_ = notify_session.Pass();

  // Sends an invite to connect signal if ready.
  SendInviteToConnectSignal();
}

void BluetoothLowEnergyConnection::StopNotifySession() {
  if (notify_session_) {
    notify_session_->Stop(base::Bind(&base::DoNothing));
    notify_session_.reset();
  }
}

void BluetoothLowEnergyConnection::SendInviteToConnectSignal() {
  if (sub_status() == SubStatus::NOTIFY_SESSION_READY) {
    VLOG(1) << "Sending invite to connect signal";
    SetSubStatus(SubStatus::WAITING_RESPONSE_SIGNAL);

    WriteRequest write_request = BuildWriteRequest(
        ToByteVector(
            static_cast<uint32>(ControlSignal::kInviteToConnectSignal)),
        std::vector<uint8>(), false);

    WriteRemoteCharacteristic(write_request);
  }
}

void BluetoothLowEnergyConnection::WriteRemoteCharacteristic(
    WriteRequest request) {
  write_requests_queue_.push(request);
  ProcessNextWriteRequest();
}

void BluetoothLowEnergyConnection::ProcessNextWriteRequest() {
  BluetoothGattCharacteristic* characteristic =
      GetGattCharacteristic(to_peripheral_char_.id);
  if (!write_requests_queue_.empty() && !write_remote_characteristic_pending_ &&
      characteristic) {
    write_remote_characteristic_pending_ = true;
    WriteRequest next_request = write_requests_queue_.front();
    characteristic->WriteRemoteCharacteristic(
        next_request.value,
        base::Bind(&BluetoothLowEnergyConnection::OnRemoteCharacteristicWritten,
                   weak_ptr_factory_.GetWeakPtr(),
                   next_request.is_last_write_for_wire_message),
        base::Bind(
            &BluetoothLowEnergyConnection::OnWriteRemoteCharacteristicError,
            weak_ptr_factory_.GetWeakPtr(),
            next_request.is_last_write_for_wire_message));
  }
}

void BluetoothLowEnergyConnection::OnRemoteCharacteristicWritten(
    bool run_did_send_message_callback) {
  write_remote_characteristic_pending_ = false;
  // TODO(sacomoto): Actually pass the current message to the observer.
  if (run_did_send_message_callback)
    OnDidSendMessage(FakeWireMessage(""), true);

  // Removes the top of queue (already processed) and process the next request.
  DCHECK(!write_requests_queue_.empty());
  write_requests_queue_.pop();
  ProcessNextWriteRequest();
}

void BluetoothLowEnergyConnection::OnWriteRemoteCharacteristicError(
    bool run_did_send_message_callback,
    BluetoothGattService::GattErrorCode error) {
  VLOG(1) << "Error " << error << " writing characteristic: "
          << to_peripheral_char_.uuid.canonical_value();
  write_remote_characteristic_pending_ = false;
  // TODO(sacomoto): Actually pass the current message to the observer.
  if (run_did_send_message_callback)
    OnDidSendMessage(FakeWireMessage(""), false);

  // Increases the number of failed attempts and retry.
  DCHECK(!write_requests_queue_.empty());
  if (++write_requests_queue_.front().number_of_failed_attempts >=
      max_number_of_write_attempts_) {
    Disconnect();
    return;
  }
  ProcessNextWriteRequest();
}

BluetoothLowEnergyConnection::WriteRequest
BluetoothLowEnergyConnection::BuildWriteRequest(
    const std::vector<uint8>& signal,
    const std::vector<uint8>& bytes,
    bool is_last_write_for_wire_message) {
  std::vector<uint8> value(signal.begin(), signal.end());
  value.insert(value.end(), bytes.begin(), bytes.end());
  return WriteRequest(value, is_last_write_for_wire_message);
}

void BluetoothLowEnergyConnection::ClearWriteRequestsQueue() {
  while (!write_requests_queue_.empty())
    write_requests_queue_.pop();
}

const std::string& BluetoothLowEnergyConnection::GetRemoteDeviceAddress() {
  return remote_device().bluetooth_address;
}

BluetoothDevice* BluetoothLowEnergyConnection::GetRemoteDevice() {
  if (!adapter_ || !adapter_->IsInitialized()) {
    VLOG(1) << "adapter not ready";
    return NULL;
  }
  return adapter_->GetDevice(GetRemoteDeviceAddress());
}

BluetoothGattService* BluetoothLowEnergyConnection::GetRemoteService() {
  BluetoothDevice* remote_device = GetRemoteDevice();
  if (!remote_device) {
    VLOG(1) << "device not found";
    return NULL;
  }
  if (remote_service_.id.empty()) {
    std::vector<BluetoothGattService*> services =
        remote_device->GetGattServices();
    for (const auto& service : services)
      if (service->GetUUID() == remote_service_.uuid) {
        remote_service_.id = service->GetIdentifier();
        break;
      }
  }
  return remote_device->GetGattService(remote_service_.id);
}

BluetoothGattCharacteristic*
BluetoothLowEnergyConnection::GetGattCharacteristic(
    const std::string& gatt_characteristic) {
  BluetoothGattService* remote_service = GetRemoteService();
  if (!remote_service) {
    VLOG(1) << "service not found";
    return NULL;
  }
  return remote_service->GetCharacteristic(gatt_characteristic);
}

// TODO(sacomoto): make this robust to byte ordering in both sides of the
// SmartLock BLE socket.
uint32 BluetoothLowEnergyConnection::ToUint32(const std::vector<uint8>& bytes) {
  return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
}

// TODO(sacomoto): make this robust to byte ordering in both sides of the
// SmartLock BLE socket.
const std::vector<uint8> BluetoothLowEnergyConnection::ToByteVector(
    const uint32 value) {
  std::vector<uint8> bytes(4, 0);
  bytes[0] = static_cast<uint8>(value);
  bytes[1] = static_cast<uint8>(value >> 8);
  bytes[2] = static_cast<uint8>(value >> 16);
  bytes[3] = static_cast<uint8>(value >> 24);
  return bytes;
}

}  // namespace proximity_auth
