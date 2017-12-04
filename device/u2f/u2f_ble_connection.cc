// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/u2f_ble_connection.h"

#include <iterator>
#include <utility>

#include "base/barrier_closure.h"
#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "device/bluetooth/adapter.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_gatt_connection.h"
#include "device/bluetooth/bluetooth_gatt_notify_session.h"
#include "device/bluetooth/bluetooth_remote_gatt_characteristic.h"
#include "device/bluetooth/bluetooth_uuid.h"
#include "device/bluetooth/public/interfaces/gatt_result_type_converter.h"
#include "device/u2f/u2f_ble_uuids.h"

namespace device {

U2fBleConnection::U2fBleConnection(std::string device_address,
                                   ConnectCallback connect_callback,
                                   ReadCallback read_callback)
    : address_(std::move(device_address)),
      connect_callback_(connect_callback),
      read_callback_(std::move(read_callback)),
      weak_factory_(this) {
  factory_.GetAdapter(base::BindOnce(&U2fBleConnection::OnGetAdapter,
                                     weak_factory_.GetWeakPtr()));
}

U2fBleConnection::~U2fBleConnection() {
  if (legacy_adapter_)
    legacy_adapter_->RemoveObserver(this);
}

void U2fBleConnection::ReadControlPointLength(
    ControlPointLengthCallback callback) {
  if (!device_) {
    LOG(ERROR) << "No device present.";
    std::move(callback).Run(base::nullopt);
    return;
  }

  device_->ReadValueForCharacteristic(
      u2f_service_->id, control_point_length_->id,
      base::BindOnce(OnReadControlPointLength, std::move(callback)));
}

void U2fBleConnection::ReadServiceRevisions(ServiceRevisionsCallback callback) {
  if (!device_) {
    LOG(ERROR) << "No device present.";
    std::move(callback).Run({});
    return;
  }

  if (!service_revision_ && !service_revision_bitfield_) {
    LOG(ERROR) << "Service Revision Characteristics do not exist.";
    std::move(callback).Run({});
    return;
  }

  // Start from a clean state.
  service_revisions_.clear();

  // In order to obtain the full set of supported service revisions it is
  // possible that both the |service_revision_| and |service_revision_bitfield_|
  // characteristics must be read. These reads can happen in parallel, but both
  // must finish before a result can be returned. Hence a BarrierClosure is
  // introduced invoking ReturnServiceRevisions() once both characteristic reads
  // are done.
  base::OnceClosure return_callback =
      base::BindOnce(&U2fBleConnection::ReturnServiceRevisions,
                     weak_factory_.GetWeakPtr(), std::move(callback));

  if (!service_revision_) {
    device_->ReadValueForCharacteristic(
        u2f_service_->id, service_revision_bitfield_->id,
        base::BindOnce(&U2fBleConnection::OnReadServiceRevisionBitfied,
                       weak_factory_.GetWeakPtr(), std::move(return_callback)));
    return;
  }

  if (!service_revision_bitfield_) {
    device_->ReadValueForCharacteristic(
        u2f_service_->id, service_revision_->id,
        base::BindOnce(&U2fBleConnection::OnReadServiceRevision,
                       weak_factory_.GetWeakPtr(), std::move(return_callback)));
    return;
  }

  base::RepeatingClosure barrier_closure =
      base::BarrierClosure(2, std::move(return_callback));

  device_->ReadValueForCharacteristic(
      u2f_service_->id, service_revision_->id,
      base::BindOnce(&U2fBleConnection::OnReadServiceRevision,
                     weak_factory_.GetWeakPtr(), barrier_closure));

  device_->ReadValueForCharacteristic(
      u2f_service_->id, service_revision_bitfield_->id,
      base::BindOnce(&U2fBleConnection::OnReadServiceRevisionBitfied,
                     weak_factory_.GetWeakPtr(), barrier_closure));
}

void U2fBleConnection::WriteServiceRevision(ServiceRevision service_revision,
                                            WriteCallback callback) {
  if (!device_) {
    LOG(ERROR) << "No device present.";
    std::move(callback).Run(false);
    return;
  }

  if (!service_revision_bitfield_) {
    LOG(ERROR) << "Write Service Revision Failed: Service Revision Bitfield "
                  "characteristic not present.";
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

  device_->WriteValueForCharacteristic(
      u2f_service_->id, service_revision_bitfield_->id, payload,
      base::BindOnce(OnWrite, std::move(callback)));
}

void U2fBleConnection::Write(const std::vector<uint8_t>& data,
                             WriteCallback callback) {
  if (!device_) {
    LOG(ERROR) << "No device present.";
    std::move(callback).Run(false);
    return;
  }

  device_->WriteValueForCharacteristic(
      u2f_service_->id, control_point_->id, data,
      base::BindOnce(OnWrite, std::move(callback)));
}

void U2fBleConnection::OnGetAdapter(bluetooth::mojom::AdapterPtr adapter) {
  if (!adapter) {
    LOG(ERROR) << "Failed to get Adapter";
    connect_callback_.Run(false);
    return;
  }

  VLOG(2) << "Got Adapter";
  adapter_ = std::move(adapter);
  adapter_->ConnectToDevice(address_,
                            base::BindOnce(&U2fBleConnection::OnConnectToDevice,
                                           weak_factory_.GetWeakPtr()));
}

void U2fBleConnection::OnConnectToDevice(bluetooth::mojom::ConnectResult result,
                                         bluetooth::mojom::DevicePtr device) {
  if (!device) {
    LOG(ERROR) << "Failed to connect to Device: " << result;
    connect_callback_.Run(false);
    return;
  }

  VLOG(2) << "Connected to Device";
  device_ = std::move(device);
  device_.set_connection_error_handler(base::BindOnce(
      &U2fBleConnection::OnConnectionError, weak_factory_.GetWeakPtr()));
  device_->GetServices(base::BindOnce(&U2fBleConnection::OnGetServices,
                                      weak_factory_.GetWeakPtr()));
}

void U2fBleConnection::OnGetServices(
    std::vector<bluetooth::mojom::ServiceInfoPtr> services) {
  BluetoothUUID u2f_service_uuid(U2F_SERVICE_UUID);
  auto u2f_service = std::find_if(services.begin(), services.end(),
                                  [&u2f_service_uuid](const auto& service) {
                                    return service->uuid == u2f_service_uuid;
                                  });
  if (u2f_service == services.end()) {
    LOG(ERROR) << "U2F Service not found on Device.";
    OnConnectionError();
    return;
  }

  u2f_service_ = std::move(*u2f_service);
  device_->GetCharacteristics(
      u2f_service_->id, base::BindOnce(&U2fBleConnection::OnGetCharacteristics,
                                       weak_factory_.GetWeakPtr()));
}

void U2fBleConnection::OnGetCharacteristics(
    base::Optional<std::vector<bluetooth::mojom::CharacteristicInfoPtr>>
        characteristics) {
  if (!characteristics) {
    LOG(ERROR) << "No Characteristics found on Service.";
    OnConnectionError();
    return;
  }

  for (auto& characteristic : *characteristics) {
    const std::string& uuid = characteristic->uuid.canonical_value();
    if (uuid == U2F_CONTROL_POINT_LENGTH_UUID) {
      control_point_length_ = std::move(characteristic);
    } else if (uuid == U2F_CONTROL_POINT_UUID) {
      control_point_ = std::move(characteristic);
    } else if (uuid == U2F_STATUS_UUID) {
      status_ = std::move(characteristic);
    } else if (uuid == U2F_SERVICE_REVISION_UUID) {
      service_revision_ = std::move(characteristic);
    } else if (uuid == U2F_SERVICE_REVISION_BITFIELD_UUID) {
      service_revision_bitfield_ = std::move(characteristic);
    }
  }

  if (!control_point_length_ || !control_point_ || !status_ ||
      !(service_revision_ || service_revision_bitfield_)) {
    LOG(ERROR) << "U2F Characteristics missing.";
    OnConnectionError();
    return;
  }

  BluetoothAdapterFactory::GetAdapter(base::Bind(
      &U2fBleConnection::OnGetAdapterLegacy, weak_factory_.GetWeakPtr()));
}

void U2fBleConnection::OnConnectionError() {
  connect_callback_.Run(false);
  device_.reset();
}

void U2fBleConnection::DeviceAdded(BluetoothAdapter* adapter,
                                   BluetoothDevice* device) {
  if (legacy_adapter_ != adapter || device->GetAddress() != address_)
    return;

  adapter_->ConnectToDevice(address_,
                            base::BindOnce(&U2fBleConnection::OnConnectToDevice,
                                           weak_factory_.GetWeakPtr()));
}

void U2fBleConnection::DeviceAddressChanged(BluetoothAdapter* adapter,
                                            BluetoothDevice* device,
                                            const std::string& old_address) {
  if (legacy_adapter_ != adapter || old_address != address_)
    return;
  address_ = device->GetAddress();
}

void U2fBleConnection::OnGetAdapterLegacy(
    scoped_refptr<BluetoothAdapter> adapter) {
  legacy_adapter_ = std::move(adapter);
  if (!legacy_adapter_) {
    LOG(ERROR) << "Failed to get Legacy Adapter";
    OnConnectionError();
    return;
  }

  legacy_adapter_->AddObserver(this);
  BluetoothDevice* device = legacy_adapter_->GetDevice(address_);
  if (!device) {
    LOG(ERROR) << "Failed to get Legacy Device";
    OnConnectionError();
    return;
  }

  BluetoothRemoteGattService* service =
      device->GetGattService(u2f_service_->id);
  if (!service) {
    LOG(ERROR) << "Failed to get Legacy Service";
    OnConnectionError();
    return;
  }

  device::BluetoothRemoteGattCharacteristic* characteristic =
      service->GetCharacteristic(status_->id);

  if (!characteristic) {
    LOG(ERROR) << "Failed to get Legacy Characteristic";
    OnConnectionError();
    return;
  }

  characteristic->StartNotifySession(
      base::Bind(&U2fBleConnection::OnStartNotifySession,
                 weak_factory_.GetWeakPtr()),
      base::Bind(&U2fBleConnection::OnStartNotifySessionError,
                 weak_factory_.GetWeakPtr()));
}

void U2fBleConnection::GattCharacteristicValueChanged(
    BluetoothAdapter* adapter,
    BluetoothRemoteGattCharacteristic* characteristic,
    const std::vector<uint8_t>& value) {
  if (characteristic->GetIdentifier() != status_->id)
    return;
  VLOG(2) << "Status characteristic value changed.";
  read_callback_.Run(value);
}

void U2fBleConnection::OnStartNotifySession(
    std::unique_ptr<BluetoothGattNotifySession> notify_session) {
  notify_session_ = std::move(notify_session);
  VLOG(2) << "Created notification session. Connection established.";
  connect_callback_.Run(true);
}

void U2fBleConnection::OnStartNotifySessionError(
    BluetoothGattService::GattErrorCode error_code) {
  LOG(ERROR) << "StartNotifySession() failed: "
             << mojo::ConvertTo<bluetooth::mojom::GattResult>(error_code);
  OnConnectionError();
}

// static
void U2fBleConnection::OnReadControlPointLength(
    ControlPointLengthCallback callback,
    bluetooth::mojom::GattResult result,
    const base::Optional<std::vector<uint8_t>>& value) {
  if (result != bluetooth::mojom::GattResult::SUCCESS || !value) {
    LOG(ERROR) << "Error Reading Control Point Length: " << result;
    std::move(callback).Run(base::nullopt);
    return;
  }

  if (value->size() != 2) {
    LOG(ERROR) << "Wrong Control Point Length: " << value->size() << " Bytes";
    std::move(callback).Run(base::nullopt);
    return;
  }

  uint16_t length = (static_cast<uint16_t>((*value)[0]) << 8) + (*value)[1];
  VLOG(2) << "Control Point Length: " << length;
  std::move(callback).Run(length);
}

// static
void U2fBleConnection::OnReadServiceRevision(
    base::OnceClosure callback,
    bluetooth::mojom::GattResult result,
    const base::Optional<std::vector<uint8_t>>& value) {
  if (result != bluetooth::mojom::GattResult::SUCCESS || !value) {
    LOG(ERROR) << "Error Reading Service Revision: " << result;
    std::move(callback).Run();
    return;
  }

  std::string service_revision(value->begin(), value->end());
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

void U2fBleConnection::OnReadServiceRevisionBitfied(
    base::OnceClosure callback,
    bluetooth::mojom::GattResult result,
    const base::Optional<std::vector<uint8_t>>& value) {
  if (result != bluetooth::mojom::GattResult::SUCCESS || !value) {
    LOG(ERROR) << "Error Reading Service Revision Bitfield: " << result;
    std::move(callback).Run();
    return;
  }

  if (value->empty()) {
    LOG(ERROR) << "Service Revision Bitfield is empty.";
    std::move(callback).Run();
    return;
  }

  if (value->size() != 1u) {
    LOG(WARNING) << "Service Revision Bitfield has unexpected size: "
                 << value->size() << ". Ignoring all but the first byte.";
  }

  uint8_t bitset = (*value)[0];
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

void U2fBleConnection::ReturnServiceRevisions(
    ServiceRevisionsCallback callback) {
  std::move(callback).Run(std::move(service_revisions_));
}

// static
void U2fBleConnection::OnWrite(WriteCallback callback,
                               bluetooth::mojom::GattResult result) {
  if (result != bluetooth::mojom::GattResult::SUCCESS) {
    LOG(ERROR) << "Write Failed: " << result;
    std::move(callback).Run(false);
    return;
  }

  VLOG(2) << "Write succeeded.";
  std::move(callback).Run(true);
}

}  // namespace device
