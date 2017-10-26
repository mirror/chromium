// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/u2f_register.h"
#include <utility>

#include "device/u2f/u2f_discovery.h"
#include "services/service_manager/public/cpp/connector.h"

namespace device {

U2fRegister::U2fRegister(
    const std::vector<std::vector<uint8_t>>& registered_keys,
    const std::vector<uint8_t>& challenge_hash,
    const std::vector<uint8_t>& app_param,
    std::vector<std::unique_ptr<U2fDiscovery>> discoveries,
    const ResponseCallback& cb)
    : U2fRequest(std::move(discoveries), cb),
      challenge_hash_(challenge_hash),
      app_param_(app_param),
      registered_keys_(registered_keys),
      checked_exclusion_list_(false),
      weak_factory_(this) {}

U2fRegister::~U2fRegister() {}

// static
std::unique_ptr<U2fRequest> U2fRegister::TryRegistration(
    const std::vector<std::vector<uint8_t>>& registered_keys,
    const std::vector<uint8_t>& challenge_hash,
    const std::vector<uint8_t>& app_param,
    std::vector<std::unique_ptr<U2fDiscovery>> discoveries,
    const ResponseCallback& cb) {
  std::unique_ptr<U2fRequest> request = std::make_unique<U2fRegister>(
      registered_keys, challenge_hash, app_param, std::move(discoveries), cb);
  request->Start();
  return request;
}

void U2fRegister::TryDevice() {
  DCHECK(current_device_);
  // If there is an non-empty exclude list, check for duplicate registration
  // using check-only sign and then proceed with registration.
  if (registered_keys_.size() > 0 && !checked_exclusion_list_) {
    auto it = registered_keys_.cbegin();
    current_device_->Sign(
        app_param_, challenge_hash_, *it,
        base::Bind(&U2fRegister::OnTryDeviceCheckAlreadyRegistered,
                   weak_factory_.GetWeakPtr(), it),
        true);
  } else {
    // Exclusion list not provided.
    current_device_->Register(app_param_, challenge_hash_,
                              base::Bind(&U2fRegister::OnTryDevice,
                                         weak_factory_.GetWeakPtr(), false));
  }
}

void U2fRegister::OnTryDeviceCheckAlreadyRegistered(
    std::vector<std::vector<uint8_t>>::const_iterator it,
    U2fReturnCode return_code,
    const std::vector<uint8_t>& response_data) {
  switch (return_code) {
    case U2fReturnCode::SUCCESS:
    case U2fReturnCode::CONDITIONS_NOT_SATISFIED:
      // Duplicate registration found. Call bogus registration to check for
      // user presence (touch) and terminate the registration process.
      current_device_->Register(kBogusAppParam, kBogusChallenge,
                                base::Bind(&U2fRegister::OnTryDevice,
                                           weak_factory_.GetWeakPtr(), true));
      break;

    case U2fReturnCode::INVALID_PARAMS:
      // Iterate through the provided key handles in exclude list and check for
      // duplicate keys.
      if (++it != registered_keys_.end()) {
        current_device_->Sign(
            app_param_, challenge_hash_, *it,
            base::Bind(&U2fRegister::OnTryDeviceCheckAlreadyRegistered,
                       weak_factory_.GetWeakPtr(), it),
            true);
      } else {
        register_device_list_.push_back(std::move(current_device_));
        if (devices_.size() == 0) {
          // When all devices has been checked, proceed to registration.
          completeNewDeviceRegistration();
        } else {
          state_ = State::IDLE;
          Transition();
        }
      }
      break;
    default:
      // Some sort of failure occurred. Abandon this device and move on.
      state_ = State::IDLE;
      current_device_ = nullptr;
      Transition();
      break;
  }
}

void U2fRegister::completeNewDeviceRegistration() {
  while (register_device_list_.size() > 0) {
    devices_.push_back(std::move(register_device_list_.front()));
    register_device_list_.pop_front();
  }

  current_device_ = std::move(devices_.front());
  devices_.pop_front();

  checked_exclusion_list_ = true;
  current_device_->Register(
      app_param_, challenge_hash_,
      base::Bind(&U2fRegister::OnTryDevice, weak_factory_.GetWeakPtr(), false));
  return;
}

void U2fRegister::OnTryDevice(bool is_duplicate_registration,
                              U2fReturnCode return_code,
                              const std::vector<uint8_t>& response_data) {
  switch (return_code) {
    case U2fReturnCode::SUCCESS:
      state_ = State::COMPLETE;
      is_duplicate_registration
          ? cb_.Run(U2fReturnCode::CONDITIONS_NOT_SATISFIED, response_data)
          : cb_.Run(return_code, response_data);
      break;
    case U2fReturnCode::CONDITIONS_NOT_SATISFIED:
      // Waiting for user touch, move on and try this device later.
      state_ = State::IDLE;
      Transition();
      break;
    case U2fReturnCode::INVALID_PARAMS:
      cb_.Run(return_code, response_data);
      break;
    case U2fReturnCode::FAILURE:
      break;
    default:
      state_ = State::IDLE;
      // An error has occurred, quit trying this device.
      current_device_ = nullptr;
      Transition();
      break;
  }
}

}  // namespace device
