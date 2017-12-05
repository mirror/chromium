// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/u2f_ble_connection.h"

#include <utility>

#include "base/barrier_closure.h"
#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_gatt_connection.h"
#include "device/bluetooth/bluetooth_gatt_notify_session.h"
#include "device/bluetooth/bluetooth_remote_gatt_characteristic.h"
#include "device/bluetooth/bluetooth_remote_gatt_service.h"
#include "device/bluetooth/bluetooth_uuid.h"
#include "device/u2f/u2f_ble_uuids.h"

namespace device {

namespace {

constexpr const char* to_string(BluetoothDevice::ConnectErrorCode error_code) {
  switch (error_code) {
    case BluetoothDevice::ERROR_AUTH_CANCELED:
      return "ERROR_AUTH_CANCELED";
    case BluetoothDevice::ERROR_AUTH_FAILED:
      return "ERROR_AUTH_FAILED";
    case BluetoothDevice::ERROR_AUTH_REJECTED:
      return "ERROR_AUTH_REJECTED";
    case BluetoothDevice::ERROR_AUTH_TIMEOUT:
      return "ERROR_AUTH_TIMEOUT";
    case BluetoothDevice::ERROR_FAILED:
      return "ERROR_FAILED";
    case BluetoothDevice::ERROR_INPROGRESS:
      return "ERROR_INPROGRESS";
    case BluetoothDevice::ERROR_UNKNOWN:
      return "ERROR_UNKNOWN";
    case BluetoothDevice::ERROR_UNSUPPORTED_DEVICE:
      return "ERROR_UNSUPPORTED_DEVICE";
    default:
      return "";
  }
}

constexpr const char* to_string(
    BluetoothGattService::GattErrorCode error_code) {
  switch (error_code) {
    case BluetoothGattService::GATT_ERROR_UNKNOWN:
      return "GATT_ERROR_UNKNOWN";
    case BluetoothGattService::GATT_ERROR_FAILED:
      return "GATT_ERROR_FAILED";
    case BluetoothGattService::GATT_ERROR_IN_PROGRESS:
      return "GATT_ERROR_IN_PROGRESS";
    case BluetoothGattService::GATT_ERROR_INVALID_LENGTH:
      return "GATT_ERROR_INVALID_LENGTH";
    case BluetoothGattService::GATT_ERROR_NOT_PERMITTED:
      return "GATT_ERROR_NOT_PERMITTED";
    case BluetoothGattService::GATT_ERROR_NOT_AUTHORIZED:
      return "GATT_ERROR_NOT_AUTHORIZED";
    case BluetoothGattService::GATT_ERROR_NOT_PAIRED:
      return "GATT_ERROR_NOT_PAIRED";
    case BluetoothGattService::GATT_ERROR_NOT_SUPPORTED:
      return "GATT_ERROR_NOT_SUPPORTED";
  }
}

}  // namespace

U2fBleConnection::U2fBleConnection(std::string device_address,
                                   ConnectCallback connect_callback,
                                   ReadCallback read_callback)
    : address_(std::move(device_address)),
      connect_callback_(connect_callback),
      read_callback_(std::move(read_callback)),
      weak_factory_(this) {
  BluetoothAdapterFactory::GetAdapter(
      base::Bind(&U2fBleConnection::OnGetAdapter, weak_factory_.GetWeakPtr()));
}

U2fBleConnection::~U2fBleConnection() {
  if (adapter_)
    adapter_->RemoveObserver(this);
}

void U2fBleConnection::ReadControlPointLength(
    ControlPointLengthCallback callback) {
  const BluetoothRemoteGattService* u2f_service = GetU2fService();
  if (!u2f_service) {
    std::move(callback).Run(base::nullopt);
    return;
  }

  BluetoothRemoteGattCharacteristic* control_point_length =
      u2f_service->GetCharacteristic(control_point_length_id_);
  if (!control_point_length) {
    LOG(ERROR) << "No Control Point Length characteristic present.";
    std::move(callback).Run(base::nullopt);
    return;
  }

  auto copyable_callback = base::AdaptCallbackForRepeating(std::move(callback));
  control_point_length->ReadRemoteCharacteristic(
      base::Bind(OnReadControlPointLength, copyable_callback),
      base::Bind(OnReadControlPointLengthError, copyable_callback));
}

void U2fBleConnection::ReadServiceRevisions(ServiceRevisionsCallback callback) {
  const BluetoothRemoteGattService* u2f_service = GetU2fService();
  if (!u2f_service) {
    std::move(callback).Run({});
    return;
  }

  BluetoothRemoteGattCharacteristic* service_revision =
      u2f_service->GetCharacteristic(service_revision_id_);

  BluetoothRemoteGattCharacteristic* service_revision_bitfield =
      u2f_service->GetCharacteristic(service_revision_bitfield_id_);

  if (!service_revision && !service_revision_bitfield) {
    LOG(ERROR) << "Service Revision Characteristics do not exist.";
    std::move(callback).Run({});
    return;
  }

  // Start from a clean state.
  service_revisions_.clear();

  // In order to obtain the full set of supported service revisions it is
  // possible that both the |service_revision_| and |service_revision_bitfield_|
  // characteristics must be read. Potentially we need to take the union of
  // the individually supported service revisions, hence the indirection to
  // ReturnServiceRevisions() is introduced.
  base::Closure copyable_callback = base::AdaptCallbackForRepeating(
      base::BindOnce(&U2fBleConnection::ReturnServiceRevisions,
                     weak_factory_.GetWeakPtr(), std::move(callback)));

  // If the Service Revision Bitfield characteristic is not present, only
  // attempt to read the Service Revision characteristic.
  if (!service_revision_bitfield) {
    service_revision->ReadRemoteCharacteristic(
        base::Bind(&U2fBleConnection::OnReadServiceRevision,
                   weak_factory_.GetWeakPtr(), copyable_callback),
        base::Bind(&U2fBleConnection::OnReadServiceRevisionError,
                   weak_factory_.GetWeakPtr(), copyable_callback));
    return;
  }

  // If the Service Revision characteristic is not present, only
  // attempt to read the Service Revision Bitfield characteristic.
  if (!service_revision) {
    service_revision_bitfield->ReadRemoteCharacteristic(
        base::Bind(&U2fBleConnection::OnReadServiceRevisionBitfield,
                   weak_factory_.GetWeakPtr(), copyable_callback),
        base::Bind(&U2fBleConnection::OnReadServiceRevisionBitfieldError,
                   weak_factory_.GetWeakPtr(), copyable_callback));
    return;
  }

  // This is the case where both characteristics are present. These reads can
  // happen in parallel, but both must finish before a result can be returned.
  // Hence a BarrierClosure is introduced invoking ReturnServiceRevisions() once
  // both characteristic reads are done.
  base::RepeatingClosure barrier_closure =
      base::BarrierClosure(2, copyable_callback);

  service_revision->ReadRemoteCharacteristic(
      base::Bind(&U2fBleConnection::OnReadServiceRevision,
                 weak_factory_.GetWeakPtr(), barrier_closure),
      base::Bind(&U2fBleConnection::OnReadServiceRevisionError,
                 weak_factory_.GetWeakPtr(), barrier_closure));

  service_revision_bitfield->ReadRemoteCharacteristic(
      base::Bind(&U2fBleConnection::OnReadServiceRevisionBitfield,
                 weak_factory_.GetWeakPtr(), barrier_closure),
      base::Bind(&U2fBleConnection::OnReadServiceRevisionBitfieldError,
                 weak_factory_.GetWeakPtr(), barrier_closure));
}

void U2fBleConnection::WriteControlPoint(const std::vector<uint8_t>& data,
                                         WriteCallback callback) {
  const BluetoothRemoteGattService* u2f_service = GetU2fService();
  if (!u2f_service) {
    std::move(callback).Run(false);
    return;
  }

  BluetoothRemoteGattCharacteristic* control_point =
      u2f_service->GetCharacteristic(control_point_id_);
  if (!control_point) {
    LOG(ERROR) << "Control Point characteristic not present.";
    std::move(callback).Run(false);
    return;
  }

  auto copyable_callback = base::AdaptCallbackForRepeating(std::move(callback));
  control_point->WriteRemoteCharacteristic(
      data, base::Bind(OnWrite, copyable_callback),
      base::Bind(OnWriteError, copyable_callback));
}

void U2fBleConnection::WriteServiceRevision(ServiceRevision service_revision,
                                            WriteCallback callback) {
  const BluetoothRemoteGattService* u2f_service = GetU2fService();
  if (!u2f_service) {
    std::move(callback).Run(false);
    return;
  }

  BluetoothRemoteGattCharacteristic* service_revision_bitfield =
      u2f_service->GetCharacteristic(service_revision_bitfield_id_);
  if (!service_revision_bitfield) {
    LOG(ERROR) << "Service Revision Bitfield characteristic not present.";
    std::move(callback).Run(false);
    return;
  }

  std::vector<uint8_t> payload;
  switch (service_revision) {
    case ServiceRevision::VERSION_1_1:
      payload.push_back(0x80);
      break;
    case ServiceRevision::VERSION_1_2:
      payload.push_back(0x40);
      break;
    default:
      LOG(ERROR)
          << "Write Service Revision Failed: Unsupported Service Revision.";
      std::move(callback).Run(false);
      return;
  }

  auto copyable_callback = base::AdaptCallbackForRepeating(std::move(callback));
  service_revision_bitfield->WriteRemoteCharacteristic(
      payload, base::Bind(OnWrite, copyable_callback),
      base::Bind(OnWriteError, copyable_callback));
}

void U2fBleConnection::OnGetAdapter(scoped_refptr<BluetoothAdapter> adapter) {
  if (!adapter) {
    LOG(ERROR) << "Failed to get Adapter.";
    OnConnectionError();
    return;
  }

  VLOG(2) << "Got Adapter: " << adapter->GetAddress();
  adapter_ = std::move(adapter);
  adapter_->AddObserver(this);
  CreateGattConnection();
}

void U2fBleConnection::CreateGattConnection() {
  BluetoothDevice* device = adapter_->GetDevice(address_);
  if (!device) {
    LOG(ERROR) << "Failed to get Device.";
    OnConnectionError();
    return;
  }

  device->CreateGattConnection(
      base::Bind(&U2fBleConnection::OnCreateGattConnection,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&U2fBleConnection::OnCreateGattConnectionError,
                 weak_factory_.GetWeakPtr()));
}

void U2fBleConnection::OnCreateGattConnection(
    std::unique_ptr<BluetoothGattConnection> connection) {
  connection_ = std::move(connection);

  BluetoothDevice* device = adapter_->GetDevice(address_);
  if (!device) {
    LOG(ERROR) << "Failed to get Device.";
    OnConnectionError();
    return;
  }

  std::vector<BluetoothRemoteGattService*> services =
      device->GetPrimaryServicesByUUID(BluetoothUUID(U2F_SERVICE_UUID));
  if (services.empty()) {
    LOG(ERROR) << "Failed to get U2F Service.";
    OnConnectionError();
    return;
  }

  u2f_service_id_ = services.front()->GetIdentifier();
  if (services.size() > 1) {
    VLOG(2) << "Found multiple U2F Services, using first one: "
            << u2f_service_id_;
  }

  for (const auto* characteristic : services.front()->GetCharacteristics()) {
    const std::string uuid = characteristic->GetUUID().canonical_value();
    if (uuid == U2F_CONTROL_POINT_LENGTH_UUID) {
      control_point_length_id_ = characteristic->GetIdentifier();
    } else if (uuid == U2F_CONTROL_POINT_UUID) {
      control_point_id_ = characteristic->GetIdentifier();
    } else if (uuid == U2F_STATUS_UUID) {
      status_id_ = characteristic->GetIdentifier();
    } else if (uuid == U2F_SERVICE_REVISION_UUID) {
      service_revision_id_ = characteristic->GetIdentifier();
    } else if (uuid == U2F_SERVICE_REVISION_BITFIELD_UUID) {
      service_revision_bitfield_id_ = characteristic->GetIdentifier();
    }
  }

  if (control_point_length_id_.empty() || control_point_id_.empty() ||
      status_id_.empty() ||
      (service_revision_id_.empty() && service_revision_bitfield_id_.empty())) {
    LOG(ERROR) << "U2F characteristics missing.";
    OnConnectionError();
    return;
  }

  services.front()
      ->GetCharacteristic(status_id_)
      ->StartNotifySession(
          base::Bind(&U2fBleConnection::OnStartNotifySession,
                     weak_factory_.GetWeakPtr()),
          base::Bind(&U2fBleConnection::OnStartNotifySessionError,
                     weak_factory_.GetWeakPtr()));
}

void U2fBleConnection::OnCreateGattConnectionError(
    BluetoothDevice::ConnectErrorCode error_code) {
  LOG(ERROR) << "CreateGattConnection() failed: " << to_string(error_code);
  OnConnectionError();
}

void U2fBleConnection::OnStartNotifySession(
    std::unique_ptr<BluetoothGattNotifySession> notify_session) {
  notify_session_ = std::move(notify_session);
  VLOG(2) << "Created notification session. Connection established.";
  connect_callback_.Run(true);
}

void U2fBleConnection::OnStartNotifySessionError(
    BluetoothGattService::GattErrorCode error_code) {
  LOG(ERROR) << "StartNotifySession() failed: " << to_string(error_code);
  OnConnectionError();
}

void U2fBleConnection::OnConnectionError() {
  connect_callback_.Run(false);

  connection_.reset();
  notify_session_.reset();

  u2f_service_id_.clear();
  control_point_length_id_.clear();
  control_point_id_.clear();
  status_id_.clear();
  service_revision_id_.clear();
  service_revision_bitfield_id_.clear();
}

const BluetoothRemoteGattService* U2fBleConnection::GetU2fService() const {
  if (!adapter_) {
    LOG(ERROR) << "No adapter present.";
    return nullptr;
  }

  const BluetoothDevice* device = adapter_->GetDevice(address_);
  if (!device) {
    LOG(ERROR) << "No device present.";
    return nullptr;
  }

  const BluetoothRemoteGattService* u2f_service =
      device->GetGattService(u2f_service_id_);
  if (!u2f_service) {
    LOG(ERROR) << "No U2F service present.";
    return nullptr;
  }

  return u2f_service;
}

void U2fBleConnection::DeviceAdded(BluetoothAdapter* adapter,
                                   BluetoothDevice* device) {
  if (adapter != adapter_ || device->GetAddress() != address_)
    return;
  CreateGattConnection();
}

void U2fBleConnection::DeviceAddressChanged(BluetoothAdapter* adapter,
                                            BluetoothDevice* device,
                                            const std::string& old_address) {
  if (adapter != adapter_ || old_address != address_)
    return;
  address_ = device->GetAddress();
}

void U2fBleConnection::DeviceChanged(BluetoothAdapter* adapter,
                                     BluetoothDevice* device) {
  if (adapter != adapter_ || device->GetAddress() != address_)
    return;
  if (!device->IsGattConnected()) {
    LOG(ERROR) << "GATT Disconnected: " << device->GetAddress();
    OnConnectionError();
  }
}

void U2fBleConnection::GattCharacteristicValueChanged(
    BluetoothAdapter* adapter,
    BluetoothRemoteGattCharacteristic* characteristic,
    const std::vector<uint8_t>& value) {
  if (characteristic->GetIdentifier() != status_id_)
    return;
  VLOG(2) << "Status characteristic value changed.";
  read_callback_.Run(value);
}

// static
void U2fBleConnection::OnReadControlPointLength(
    ControlPointLengthCallback callback,
    const std::vector<uint8_t>& value) {
  if (value.size() != 2) {
    LOG(ERROR) << "Wrong Control Point Length: " << value.size() << " Bytes";
    std::move(callback).Run(base::nullopt);
    return;
  }

  uint16_t length = (static_cast<uint16_t>(value[0]) << 8) + value[1];
  VLOG(2) << "Control Point Length: " << length;
  std::move(callback).Run(length);
}

// static
void U2fBleConnection::OnReadControlPointLengthError(
    ControlPointLengthCallback callback,
    BluetoothGattService::GattErrorCode error_code) {
  LOG(ERROR) << "Error reading Control Point Length: " << to_string(error_code);
  std::move(callback).Run(base::nullopt);
}

void U2fBleConnection::OnReadServiceRevision(
    base::OnceClosure callback,
    const std::vector<uint8_t>& value) {
  std::string service_revision(value.begin(), value.end());
  VLOG(2) << "Service Revision: " << service_revision;

  if (service_revision == "1.0") {
    service_revisions_.insert(ServiceRevision::VERSION_1_0);
  } else if (service_revision == "1.1") {
    service_revisions_.insert(ServiceRevision::VERSION_1_1);
  } else if (service_revision == "1.2") {
    service_revisions_.insert(ServiceRevision::VERSION_1_2);
  } else {
    LOG(ERROR) << "Unknown Service Revision: " << service_revision;
    std::move(callback).Run();
    return;
  }

  std::move(callback).Run();
}

void U2fBleConnection::OnReadServiceRevisionError(
    base::OnceClosure callback,
    BluetoothGattService::GattErrorCode error_code) {
  LOG(ERROR) << "Error reading Service Revision: " << to_string(error_code);
  std::move(callback).Run();
}

void U2fBleConnection::OnReadServiceRevisionBitfield(
    base::OnceClosure callback,
    const std::vector<uint8_t>& value) {
  if (value.empty()) {
    LOG(ERROR) << "Service Revision Bitfield is empty.";
    std::move(callback).Run();
    return;
  }

  if (value.size() != 1u) {
    LOG(WARNING) << "Service Revision Bitfield has unexpected size: "
                 << value.size() << ". Ignoring all but the first byte.";
  }

  const uint8_t bitset = value[0];
  if (bitset & 0x3F) {
    LOG(WARNING) << "Service Revision Bitfield has unexpected bits set: 0x"
                 << std::hex << (bitset & 0x3F)
                 << ". Ignoring all but the first two bits.";
  }

  if (bitset & 0x80) {
    service_revisions_.insert(ServiceRevision::VERSION_1_1);
    VLOG(2) << "Detected Support for Service Revision 1.1";
  }

  if (bitset & 0x40) {
    service_revisions_.insert(ServiceRevision::VERSION_1_2);
    VLOG(2) << "Detected Support for Service Revision 1.2";
  }

  std::move(callback).Run();
}

void U2fBleConnection::OnReadServiceRevisionBitfieldError(
    base::OnceClosure callback,
    BluetoothGattService::GattErrorCode error_code) {
  LOG(ERROR) << "Error reading Service Revision Bitfield: "
             << to_string(error_code);
  std::move(callback).Run();
}

void U2fBleConnection::ReturnServiceRevisions(
    ServiceRevisionsCallback callback) {
  std::move(callback).Run(std::move(service_revisions_));
}

// static
void U2fBleConnection::OnWrite(WriteCallback callback) {
  VLOG(2) << "Write succeeded.";
  std::move(callback).Run(true);
}

// static
void U2fBleConnection::OnWriteError(
    WriteCallback callback,
    BluetoothGattService::GattErrorCode error_code) {
  LOG(ERROR) << "Write Failed: " << to_string(error_code);
  std::move(callback).Run(false);
}

}  // namespace device
