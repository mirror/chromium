// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/u2f_device_request.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "device/ctap/u2f_register_param.h"
#include "device/ctap/u2f_sign_param.h"

namespace device {

U2FDeviceRequest::U2FDeviceRequest(
    const std::string& relying_paty_id,
    std::vector<std::vector<uint8_t>> registered_keys,
    std::vector<uint8_t> application_parameter,
    std::vector<uint8_t> challenge_parameter,
    base::WeakPtr<FidoDevice> device,
    FidoDevice::DeviceCallback cb)
    : relying_party_id_(relying_paty_id),
      registered_keys_(std::move(registered_keys)),
      application_parameter_(std::move(application_parameter)),
      challenge_parameter_(std::move(challenge_parameter)),
      device_(device),
      cb_(std::move(cb)) {}

U2FDeviceRequest::~U2FDeviceRequest() = default;

std::vector<uint8_t> U2FDeviceRequest::GetResponseCredentialId() const {
  return std::vector<uint8_t>();
}

void U2FDeviceRequest::DeviceRegister(bool is_bogus_registration,
                                      FidoDevice::DeviceCallback cb) {
  if (is_bogus_registration)
    return device_->Register(
        std::make_unique<U2FRegisterParam>(kU2FBogusRegisterApplicationParam,
                                           kU2FBogusRegisterChallengeParam),
        std::move(cb));

  device_->Register(std::make_unique<U2FRegisterParam>(application_parameter_,
                                                       challenge_parameter_),
                    std::move(cb));
}

void U2FDeviceRequest::DeviceSign(std::vector<uint8_t> key_handle,
                                  bool check_only,
                                  FidoDevice::DeviceCallback cb) {
  auto sign_param = std::make_unique<U2FSignParam>(
      application_parameter_, challenge_parameter_, key_handle);
  sign_param->SetCheckOnly(check_only);

  device_->Sign(std::move(sign_param), std::move(cb));
}

}  // namespace device
