// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/device_capability_manager.h"

#include "components/cryptauth/proto/cryptauth_api.pb.h"

namespace cryptauth {

DeviceCapabilityManager::DeviceCapabilityManager(
    std::unique_ptr<CryptAuthClientFactory> cryptauth_client_factory)
    : crypt_auth_client_factory_(cryptauth_client_factory.get()),
      weak_ptr_factory_(this) {}

DeviceCapabilityManager::~DeviceCapabilityManager() {}

/* PUBLIC INTERFACE */
void DeviceCapabilityManager::SetCapabilityForDevice(
    const std::string& public_key,
    Capability capability,
    bool enabled,
    const base::Closure& success_callback,
    const base::Callback<void(const std::string&)>& error_callback) {
  scheduler_.push(Request(RequestType::SET_CAPABILITY, public_key, enabled,
                          success_callback, error_callback));
  PingScheduler();
}

void DeviceCapabilityManager::FindEligibleDevices(
    Capability capability,
    const base::Callback<
        void(const ::google::protobuf::RepeatedPtrField<ExternalDeviceInfo>&)>&
        success_callback,
    const base::Callback<void(const std::string&)>& error_callback) {
  scheduler_.push(Request(RequestType::FIND_ELIGIBLE_DEVICES, success_callback,
                          error_callback));
  PingScheduler();
}

/* PRIVATE */
DeviceCapabilityManager::Request::Request(
    RequestType request_type,
    std::string public_key,
    bool enabled,
    const base::Closure& success_callback,
    const base::Callback<void(const std::string&)>& error_callback)
    : request_type_(request_type),
      public_key_(public_key),
      enabled_(enabled),
      success_callback_(success_callback),
      error_callback_(error_callback) {}

DeviceCapabilityManager::Request::Request(
    RequestType request_type,
    const base::Callback<
        void(const ::google::protobuf::RepeatedPtrField<ExternalDeviceInfo>&)>&
        success_callback,
    const base::Callback<void(const std::string&)>& error_callback)
    : request_type_(request_type),
      success_callback_vector_arg_(success_callback),
      error_callback_(error_callback) {}

DeviceCapabilityManager::Request::~Request() {}

void DeviceCapabilityManager::ToggleEasyUnlock(
    const std::string& public_key,
    bool enabled,
    const base::Closure& success_callback,
    const base::Callback<void(const std::string&)>& error_callback) {
  std::unique_ptr<CryptAuthClient> cryptauth_client =
      crypt_auth_client_factory_->CreateInstance();
  ToggleEasyUnlockRequest request;
  request.set_enable(enabled);
  request.set_apply_to_all(true);
  request.set_public_key(public_key);
  cryptauth_client->ToggleEasyUnlock(
      request,
      base::Bind(&DeviceCapabilityManager::OnToggleEasyUnlockResponse,
                 weak_ptr_factory_.GetWeakPtr(), success_callback),
      base::Bind(&DeviceCapabilityManager::OnErrorResponse,
                 weak_ptr_factory_.GetWeakPtr(), error_callback));
}

void DeviceCapabilityManager::OnToggleEasyUnlockResponse(
    const base::Closure& original_callback,
    const ToggleEasyUnlockResponse& response) {
  original_callback.Run();
  PingScheduler();
}

void DeviceCapabilityManager::FindEligibleForUnlockDevices(
    const base::Callback<
        void(const ::google::protobuf::RepeatedPtrField<ExternalDeviceInfo>&)>&
        success_callback,
    const base::Callback<void(const std::string&)>& error_callback) {
  std::unique_ptr<CryptAuthClient> cryptauth_client =
      crypt_auth_client_factory_->CreateInstance();
  FindEligibleUnlockDevicesRequest request;
  cryptauth_client->FindEligibleUnlockDevices(
      request,
      base::Bind(&DeviceCapabilityManager::OnEligibleDevicesResponse,
                 weak_ptr_factory_.GetWeakPtr(), success_callback),
      base::Bind(&DeviceCapabilityManager::OnErrorResponse,
                 weak_ptr_factory_.GetWeakPtr(), error_callback));
}

void DeviceCapabilityManager::OnEligibleDevicesResponse(
    const base::Callback<
        void(const ::google::protobuf::RepeatedPtrField<ExternalDeviceInfo>&)>&
        original_callback,
    const FindEligibleUnlockDevicesResponse& response) {
  original_callback.Run(response.eligible_devices());
  PingScheduler();
}

void DeviceCapabilityManager::OnErrorResponse(
    const base::Callback<void(const std::string&)>& original_callback,
    const std::string& response) {
  original_callback.Run(response);
  PingScheduler();
}

void DeviceCapabilityManager::PingScheduler() {
  // is a response guranteed?  There needs to be a timeout.
  if (!scheduler_.empty()) {
    if (scheduler_.front().request_type_ == RequestType::SET_CAPABILITY) {
      ToggleEasyUnlock(scheduler_.front().public_key_,
                       scheduler_.front().enabled_,
                       scheduler_.front().success_callback_,
                       scheduler_.front().error_callback_);
    } else if (scheduler_.front().request_type_ ==
               RequestType::FIND_ELIGIBLE_DEVICES) {
      FindEligibleForUnlockDevices(
          scheduler_.front().success_callback_vector_arg_,
          scheduler_.front().error_callback_);
    }
    scheduler_.pop();
  }
}

}  // namespace cryptauth

// order must be guranteed for the same device id for something like toggle
// for different
