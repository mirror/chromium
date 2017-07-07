// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/test/fake_remote_gatt_characteristic.h"

#include <utility>
#include <vector>

#include "base/debug/stack_trace.h"

#include "base/optional.h"
#include "base/strings/stringprintf.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_remote_gatt_service.h"
#include "device/bluetooth/bluetooth_uuid.h"
#include "device/bluetooth/public/interfaces/test/fake_bluetooth.mojom.h"
#include "device/bluetooth/test/fake_read_response.h"

namespace bluetooth {

FakeRemoteGattCharacteristic::FakeRemoteGattCharacteristic(
    const std::string& characteristic_id,
    const device::BluetoothUUID& characteristic_uuid,
    mojom::CharacteristicPropertiesPtr properties,
    device::BluetoothRemoteGattService* service)
    : characteristic_id_(characteristic_id),
      characteristic_uuid_(characteristic_uuid),
      service_(service),
      weak_ptr_factory_(this) {
  properties_ = PROPERTY_NONE;
  if (properties->broadcast)
    properties_ |= PROPERTY_BROADCAST;
  if (properties->read)
    properties_ |= PROPERTY_READ;
  if (properties->write_without_response)
    properties_ |= PROPERTY_WRITE_WITHOUT_RESPONSE;
  if (properties->write)
    properties_ |= PROPERTY_WRITE;
  if (properties->notify)
    properties_ |= PROPERTY_NOTIFY;
  if (properties->indicate)
    properties_ |= PROPERTY_INDICATE;
  if (properties->authenticated_signed_writes)
    properties_ |= PROPERTY_AUTHENTICATED_SIGNED_WRITES;
  if (properties->extended_properties)
    properties_ |= PROPERTY_EXTENDED_PROPERTIES;
}

FakeRemoteGattCharacteristic::~FakeRemoteGattCharacteristic() {}

std::string FakeRemoteGattCharacteristic::AddFakeDescriptor(
    const device::BluetoothUUID& descriptor_uuid) {
  FakeDescriptorMap::iterator it;
  bool inserted;

  // Attribute instance Ids need to be unique.
  std::string new_descriptor_id = base::StringPrintf(
      "%s_%zu", GetIdentifier().c_str(), ++last_descriptor_id_);

  std::tie(it, inserted) = fake_descriptors_.emplace(
      new_descriptor_id, base::MakeUnique<FakeRemoteGattDescriptor>(
                             new_descriptor_id, descriptor_uuid, this));

  DCHECK(inserted);
  return it->second->GetIdentifier();
}

void FakeRemoteGattCharacteristic::SetNextReadResponse(
    uint16_t gatt_code,
    const base::Optional<std::vector<uint8_t>>& value) {
  DCHECK(!next_read_response_);
  next_read_response_.emplace(gatt_code, value);
}

void FakeRemoteGattCharacteristic::SetNextWriteResponse(uint16_t gatt_code) {
  DCHECK(!next_write_response_);
  next_write_response_.emplace(gatt_code);
}

void FakeRemoteGattCharacteristic::SetNextSubscribeToNotificationsResponse(
    uint16_t gatt_code) {
  DCHECK(!next_subscribe_response_);
  next_subscribe_response_ = gatt_code;
}

void FakeRemoteGattCharacteristic::SetNextUnsubscribeFromNotificationsResponse(
    uint16_t gatt_code) {
  LOG(ERROR) << "SETTING RESPONSE";
  DCHECK(!next_unsubscribe_response_);
  next_unsubscribe_response_ = gatt_code;
}

void FakeRemoteGattCharacteristic::SimulateNotification(
    const std::vector<uint8_t>& value) {
  value_ = value;
  GetService()->GetDevice()->GetAdapter()->NotifyGattCharacteristicValueChanged(
      this, value_);
}

std::string FakeRemoteGattCharacteristic::GetIdentifier() const {
  return characteristic_id_;
}

device::BluetoothUUID FakeRemoteGattCharacteristic::GetUUID() const {
  return characteristic_uuid_;
}

FakeRemoteGattCharacteristic::Properties
FakeRemoteGattCharacteristic::GetProperties() const {
  return properties_;
}

FakeRemoteGattCharacteristic::Permissions
FakeRemoteGattCharacteristic::GetPermissions() const {
  NOTREACHED();
  return PERMISSION_NONE;
}

const std::vector<uint8_t>& FakeRemoteGattCharacteristic::GetValue() const {
  NOTREACHED();
  return value_;
}

device::BluetoothRemoteGattService* FakeRemoteGattCharacteristic::GetService()
    const {
  return service_;
}

std::vector<device::BluetoothRemoteGattDescriptor*>
FakeRemoteGattCharacteristic::GetDescriptors() const {
  std::vector<device::BluetoothRemoteGattDescriptor*> descriptors;
  for (const auto& it : fake_descriptors_)
    descriptors.push_back(it.second.get());
  return descriptors;
}

device::BluetoothRemoteGattDescriptor*
FakeRemoteGattCharacteristic::GetDescriptor(
    const std::string& identifier) const {
  const auto& it = fake_descriptors_.find(identifier);
  if (it == fake_descriptors_.end())
    return nullptr;

  return it->second.get();
}

void FakeRemoteGattCharacteristic::ReadRemoteCharacteristic(
    const ValueCallback& callback,
    const ErrorCallback& error_callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&FakeRemoteGattCharacteristic::DispatchReadResponse,
                 weak_ptr_factory_.GetWeakPtr(), callback, error_callback));
}

void FakeRemoteGattCharacteristic::WriteRemoteCharacteristic(
    const std::vector<uint8_t>& value,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  // It doesn't make sense to dispatch a custom write response if the
  // characteristic only supports write without response but we still need to
  // run the callback because that's the guarantee the API makes.
  if (properties_ & PROPERTY_WRITE_WITHOUT_RESPONSE) {
    last_written_value_ = value;
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, callback);
    return;
  }

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&FakeRemoteGattCharacteristic::DispatchWriteResponse,
                 weak_ptr_factory_.GetWeakPtr(), callback, error_callback,
                 value));
}

void FakeRemoteGattCharacteristic::SubscribeToNotifications(
    device::BluetoothRemoteGattDescriptor* ccc_descriptor,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&FakeRemoteGattCharacteristic::
                     DispatchSubscribeToNotificationsResponse,
                 weak_ptr_factory_.GetWeakPtr(), callback, error_callback));
}

void FakeRemoteGattCharacteristic::UnsubscribeFromNotifications(
    device::BluetoothRemoteGattDescriptor* ccc_descriptor,
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  LOG(ERROR) << "Unsubscribing: \n" << base::debug::StackTrace().ToString();
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&FakeRemoteGattCharacteristic::
                     DispatchUnsubscribeFromNotificationsResponse,
                 weak_ptr_factory_.GetWeakPtr(), callback, error_callback));
}

void FakeRemoteGattCharacteristic::DispatchReadResponse(
    const ValueCallback& callback,
    const ErrorCallback& error_callback) {
  DCHECK(next_read_response_);
  uint16_t gatt_code = next_read_response_->gatt_code();
  base::Optional<std::vector<uint8_t>> value = next_read_response_->value();
  next_read_response_.reset();

  if (gatt_code == mojom::kGATTSuccess) {
    DCHECK(value);
    value_ = std::move(value.value());
    callback.Run(value_);
    return;
  } else if (gatt_code == mojom::kGATTInvalidHandle) {
    DCHECK(!value);
    error_callback.Run(device::BluetoothGattService::GATT_ERROR_FAILED);
    return;
  }
}

void FakeRemoteGattCharacteristic::DispatchWriteResponse(
    const base::Closure& callback,
    const ErrorCallback& error_callback,
    const std::vector<uint8_t>& value) {
  DCHECK(next_write_response_);
  uint16_t gatt_code = next_write_response_.value();
  next_write_response_.reset();

  if (gatt_code == mojom::kGATTSuccess) {
    last_written_value_ = value;
    callback.Run();
    return;
  } else if (gatt_code == mojom::kGATTInvalidHandle) {
    error_callback.Run(device::BluetoothGattService::GATT_ERROR_FAILED);
    return;
  }
}

void FakeRemoteGattCharacteristic::DispatchSubscribeToNotificationsResponse(
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  DCHECK(next_subscribe_response_);
  uint16_t gatt_code = next_subscribe_response_.value();
  next_subscribe_response_.reset();

  if (gatt_code == mojom::kGATTSuccess) {
    callback.Run();
    return;
  } else if (gatt_code == mojom::kGATTInvalidHandle) {
    error_callback.Run(device::BluetoothGattService::GATT_ERROR_FAILED);
    return;
  }
}

void FakeRemoteGattCharacteristic::DispatchUnsubscribeFromNotificationsResponse(
    const base::Closure& callback,
    const ErrorCallback& error_callback) {
  LOG(ERROR) << "Unsubscribing" << base::debug::StackTrace().ToString();
  DCHECK(next_unsubscribe_response_);
  uint16_t gatt_code = next_unsubscribe_response_.value();
  next_unsubscribe_response_.reset();

  if (gatt_code == mojom::kGATTSuccess) {
    callback.Run();
    return;
  } else if (gatt_code == mojom::kGATTInvalidHandle) {
    error_callback.Run(device::BluetoothGattService::GATT_ERROR_FAILED);
    return;
  }
}

}  // namespace bluetooth
