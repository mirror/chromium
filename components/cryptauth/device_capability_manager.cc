// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/device_capability_manager.h"

#include "base/logging.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"

namespace cryptauth {

DeviceCapabilityManager::DeviceCapabilityManager(
    CryptAuthClientFactory* cryptauth_client_factory)
    : crypt_auth_client_factory_(cryptauth_client_factory),
      weak_ptr_factory_(this) {}

DeviceCapabilityManager::~DeviceCapabilityManager() {}

void DeviceCapabilityManager::SetCapabilityForDevice(
    const std::string& public_key,
    Capability capability,
    bool enabled,
    const base::Closure& success_callback,
    const base::Callback<void(const std::string&)>& error_callback) {
  DCHECK(capability == Capability::CAPABILITY_UNLOCK_KEY);
  pending_requests_.emplace(base::MakeUnique<Request>(
      RequestType::SET_CAPABILITY, Capability::CAPABILITY_UNLOCK_KEY,
      public_key, enabled, success_callback, error_callback));
  ProcessRequestQueue();
}

void DeviceCapabilityManager::FindEligibleDevices(
    const base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                              const std::vector<IneligibleDevice>&)>&
        success_callback,
    const base::Callback<void(const std::string&)>& error_callback) {
  pending_requests_.emplace(base::MakeUnique<Request>(
      RequestType::FIND_ELIGIBLE_DEVICES, success_callback, error_callback));
  ProcessRequestQueue();
}

DeviceCapabilityManager::Request::Request() {}

DeviceCapabilityManager::Request::Request(
    RequestType request_type,
    Capability capability,
    std::string public_key,
    bool enabled,
    const base::Closure& set_capability_callback,
    const base::Callback<void(const std::string&)>& error_callback)
    : request_type_(request_type),
      public_key_(public_key),
      error_callback_(error_callback),
      capability_(capability),
      set_capability_callback_(set_capability_callback),
      enabled_(enabled) {}

DeviceCapabilityManager::Request::Request(
    RequestType request_type,
    const base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                              const std::vector<IneligibleDevice>&)>&
        find_eligible_devices_callback,
    const base::Callback<void(const std::string&)>& error_callback)
    : request_type_(request_type),
      error_callback_(error_callback),
      find_eligible_devices_callback_(find_eligible_devices_callback) {}

DeviceCapabilityManager::Request::~Request() {}

void DeviceCapabilityManager::SetUnlockKeyCapability() {
  current_cryptauth_client_ = crypt_auth_client_factory_->CreateInstance();
  ToggleEasyUnlockRequest request;
  request.set_enable(current_request_->enabled_);
  request.set_apply_to_all(true);
  request.set_public_key(current_request_->public_key_);
  current_cryptauth_client_->ToggleEasyUnlock(
      request,
      base::Bind(&DeviceCapabilityManager::OnToggleEasyUnlockResponse,
                 weak_ptr_factory_.GetWeakPtr(),
                 current_request_->set_capability_callback_),
      base::Bind(&DeviceCapabilityManager::OnErrorResponse,
                 weak_ptr_factory_.GetWeakPtr(),
                 current_request_->error_callback_));
}

void DeviceCapabilityManager::OnToggleEasyUnlockResponse(
    const base::Closure& original_callback,
    const ToggleEasyUnlockResponse& response) {
  current_cryptauth_client_.reset();
  original_callback.Run();
  current_request_.reset();
  ProcessRequestQueue();
}

void DeviceCapabilityManager::ProcessFindEligibleUnlockKeys() {
  current_cryptauth_client_ = crypt_auth_client_factory_->CreateInstance();
  FindEligibleUnlockDevicesRequest request;
  current_cryptauth_client_->FindEligibleUnlockDevices(
      request,
      base::Bind(&DeviceCapabilityManager::OnFindEligibleUnlockDevicesResponse,
                 weak_ptr_factory_.GetWeakPtr(),
                 current_request_->find_eligible_devices_callback_),
      base::Bind(&DeviceCapabilityManager::OnErrorResponse,
                 weak_ptr_factory_.GetWeakPtr(),
                 current_request_->error_callback_));
}

void DeviceCapabilityManager::OnFindEligibleUnlockDevicesResponse(
    const base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                              const std::vector<IneligibleDevice>&)>&
        original_callback,
    const FindEligibleUnlockDevicesResponse& response) {
  current_cryptauth_client_.reset();
  original_callback.Run(
      std::vector<ExternalDeviceInfo>(response.eligible_devices().begin(),
                                      response.eligible_devices().end()),
      std::vector<IneligibleDevice>(response.ineligible_devices().begin(),
                                    response.ineligible_devices().end()));
  current_request_.reset();
  ProcessRequestQueue();
}

void DeviceCapabilityManager::OnErrorResponse(
    const base::Callback<void(const std::string&)>& original_callback,
    const std::string& response) {
  current_cryptauth_client_.reset();
  original_callback.Run(response);
  current_request_.reset();
  ProcessRequestQueue();
}

void DeviceCapabilityManager::ProcessSetCapabilityRequest() {
  DCHECK(current_request_->capability_ == Capability::CAPABILITY_UNLOCK_KEY);
  SetUnlockKeyCapability();
}

void DeviceCapabilityManager::ProcessRequestQueue() {
  if (current_request_ || pending_requests_.empty())
    return;
  current_request_ = std::move(pending_requests_.front());
  pending_requests_.pop();
  switch (current_request_->request_type_) {
    case RequestType::SET_CAPABILITY:
      ProcessSetCapabilityRequest();
      return;
    case RequestType::FIND_ELIGIBLE_DEVICES:
      ProcessFindEligibleUnlockKeys();
      return;
    default:
      NOTREACHED();
      return;
  }
}

}  // namespace cryptauth