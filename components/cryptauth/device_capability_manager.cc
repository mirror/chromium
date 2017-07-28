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
      in_progress_(false),
      weak_ptr_factory_(this) {}

DeviceCapabilityManager::~DeviceCapabilityManager() {}

/* PUBLIC INTERFACE */
void DeviceCapabilityManager::SetCapabilityForDevice(
    const std::string& public_key,
    Capability capability,
    bool enabled,
    const base::Closure& success_callback,
    const base::Callback<void(const std::string&)>& error_callback) {
  if (capability == Capability::CAPABILITY_UNLOCK_KEY) {
    scheduler_.push(Request(RequestType::SET_CAPABILITY,
                            Capability::CAPABILITY_UNLOCK_KEY, public_key,
                            enabled, success_callback, error_callback));
  }
  PingScheduler();
}

void DeviceCapabilityManager::FindEligibleDevices(
    Capability capability,
    const base::Callback<void(const std::vector<ExternalDeviceInfo>&)>&
        success_callback,
    const base::Callback<void(const std::string&)>& error_callback) {
  // For easy unlock
  if (capability == Capability::CAPABILITY_READ_UNLOCK_DEVICES) {
    LOG(ERROR) << "FindeligibleDevices called.";
    scheduler_.push(Request(RequestType::FIND_ELIGIBLE_DEVICES,
                            Capability::CAPABILITY_READ_UNLOCK_DEVICES,
                            success_callback, error_callback));
  }
  PingScheduler();
}

/* PRIVATE */
DeviceCapabilityManager::Request::Request() {}

DeviceCapabilityManager::Request::Request(
    RequestType request_type,
    Capability capability,
    std::string public_key,
    bool enabled,
    const base::Closure& success_callback,
    const base::Callback<void(const std::string&)>& error_callback)
    : request_type_(request_type),
      capability_(capability),
      public_key_(public_key),
      enabled_(enabled),
      success_callback_(success_callback),
      error_callback_(error_callback) {}

DeviceCapabilityManager::Request::Request(
    RequestType request_type,
    Capability capability,
    const base::Callback<void(const std::vector<ExternalDeviceInfo>&)>&
        success_callback,
    const base::Callback<void(const std::string&)>& error_callback)
    : request_type_(request_type),
      capability_(capability),
      success_callback_vector_arg_(success_callback),
      error_callback_(error_callback) {}

DeviceCapabilityManager::Request::~Request() {}

DeviceCapabilityManager::Request::Request(const Request& request) {
  request_type_ = request.request_type_;
  capability_ = request.capability_;
  public_key_ = request.public_key_;
  enabled_ = request.enabled_;
  success_callback_ = request.success_callback_;
  error_callback_ = request.error_callback_;
}

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
  InvokeNextRequest();
}

void DeviceCapabilityManager::FindEligibleForUnlockDevices(
    const base::Callback<void(const std::vector<ExternalDeviceInfo>&)>&
        success_callback,
    const base::Callback<void(const std::string&)>& error_callback) {
  std::unique_ptr<CryptAuthClient> cryptauth_client =
      crypt_auth_client_factory_->CreateInstance();
  FindEligibleUnlockDevicesRequest request;
  LOG(ERROR) << "FOR REGAN - in class - FindEligibleForUnlockDevices is called";

  cryptauth_client->FindEligibleUnlockDevices(
      request,
      base::Bind(&DeviceCapabilityManager::OnEligibleDevicesResponse,
                 weak_ptr_factory_.GetWeakPtr(), success_callback),
      base::Bind(&DeviceCapabilityManager::OnErrorResponse,
                 weak_ptr_factory_.GetWeakPtr(), error_callback));
}

void DeviceCapabilityManager::OnEligibleDevicesResponse(
    const base::Callback<void(const std::vector<ExternalDeviceInfo>&)>&
        original_callback,
    const FindEligibleUnlockDevicesResponse& response) {
  LOG(ERROR) << "FOR REGAN - in class - OnEligibleDevicesResponse Callback";
  // TODO(hsuregan): Extract vector of ExternalDeviceInfo from response, then
  // invoke callback with the vector.

  // Why does this line cause a crash? is it because of the way I'm binding the
  // original callback in the unittest?
  original_callback.Run(std::vector<ExternalDeviceInfo>());

  InvokeNextRequest();
}

void DeviceCapabilityManager::OnErrorResponse(
    const base::Callback<void(const std::string&)>& original_callback,
    const std::string& response) {
  LOG(ERROR) << "FOR REGAN - in class - OnErrorResponse callback";
  original_callback.Run(response);
  InvokeNextRequest();
}

void DeviceCapabilityManager::InvokeNextRequest() {
  scheduler_.pop();
  in_progress_ = false;
  PingScheduler();
}

void DeviceCapabilityManager::PingScheduler() {
  if (!scheduler_.empty() && !in_progress_) {
    in_progress_ = true;
    if (scheduler_.front().request_type_ == RequestType::SET_CAPABILITY) {
      if (scheduler_.front().capability_ == Capability::CAPABILITY_UNLOCK_KEY) {
        ToggleEasyUnlock(scheduler_.front().public_key_,
                         scheduler_.front().enabled_,
                         scheduler_.front().success_callback_,
                         scheduler_.front().error_callback_);
      }
    } else if (scheduler_.front().request_type_ ==
               RequestType::FIND_ELIGIBLE_DEVICES) {
      if (scheduler_.front().capability_ ==
          Capability::CAPABILITY_READ_UNLOCK_DEVICES) {
        LOG(ERROR) << "REGAN from stack";
        FindEligibleForUnlockDevices(
            scheduler_.front().success_callback_vector_arg_,
            scheduler_.front().error_callback_);
      }
    }
  }
}

}  // namespace cryptauth