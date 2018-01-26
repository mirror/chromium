// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/fido_device.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread_task_runner_handle.h"
#include "crypto/sha2.h"
#include "device/ctap/ctap_authenticator_request_param.h"
#include "device/ctap/device_response_converter.h"
#include "device/ctap/u2f_device_request.h"
#include "device/u2f/u2f_apdu_response.h"

namespace device {

FidoDevice::FidoDevice() = default;
FidoDevice::~FidoDevice() = default;

void FidoDevice::MakeCredential(
    std::unique_ptr<CTAPMakeCredentialRequestParam> command,
    DeviceCallback cb,
    ConvertToU2FRequestCallback u2f_callback) {
  LOG(ERROR) << "\n\n==== sending make credential ====\n\n";
  u2f_callback_ = std::move(u2f_callback);
  DeviceTransact(std::move(command), std::move(cb));
}

void FidoDevice::GetAssertion(
    std::unique_ptr<CTAPGetAssertionRequestParam> command,
    DeviceCallback cb,
    ConvertToU2FRequestCallback u2f_callback) {
  LOG(ERROR) << "\n\n==== sending get assertion ====\n\n";
  u2f_callback_ = std::move(u2f_callback);
  DeviceTransact(std::move(command), std::move(cb));
}

void FidoDevice::GetNextAssertion(DeviceCallback cb) {
  CHECK(SupportsCTAP());
  LOG(ERROR) << "\n\n==== sending get next assertion ====\n\n";
  DeviceTransact(std::make_unique<CTAPAuthenticatorRequestParam>(
                     CTAPAuthenticatorRequestParam::
                         CreateAuthenticatorGetNextAssertionParam()),
                 std::move(cb));
}

void FidoDevice::Register(std::unique_ptr<U2FRegisterParam> command,
                          DeviceCallback callback) {
  LOG(ERROR) << "\n\n==== sending u2f register ====\n\n";

  CHECK(!SupportsCTAP());
  DeviceTransact(std::move(command), std::move(callback));
}

void FidoDevice::Sign(std::unique_ptr<U2FSignParam> command,
                      DeviceCallback callback) {
  LOG(ERROR) << "\n\n==== sending u2f sign ====\n\n";

  CHECK(!SupportsCTAP());
  DeviceTransact(std::move(command), std::move(callback));
}

void FidoDevice::GetInfo(DeviceCallback cb) {
  LOG(ERROR) << "\n\n==== sending get info ====\n\n";

  DeviceTransact(
      std::make_unique<CTAPAuthenticatorRequestParam>(
          CTAPAuthenticatorRequestParam::CreateAuthenticatorGetInfoParam()),
      std::move(cb));
}

void FidoDevice::Reset(DeviceCallback cb) {
  LOG(ERROR) << "\n\n==== sending reset ====\n\n";

  CHECK(SupportsCTAP());
  DeviceTransact(
      std::make_unique<CTAPAuthenticatorRequestParam>(
          CTAPAuthenticatorRequestParam::CreateAuthenticatorResetParam()),
      std::move(cb));
}

void FidoDevice::Cancel() {
  CHECK(SupportsCTAP());
  LOG(ERROR) << "\n\n==== sending cancel ====\n\n";

  DeviceTransact(
      std::make_unique<CTAPAuthenticatorRequestParam>(
          CTAPAuthenticatorRequestParam::CreateAuthenticatorCancelParam()),
      base::BindOnce(
          [](CTAPDeviceResponseCode response_code,
             const std::vector<uint8_t>& response_buffer) { return; }));
}

void FidoDevice::GetInfoCallback(std::unique_ptr<CTAPRequestParam> command,
                                 DeviceCallback callback,
                                 CTAPDeviceResponseCode response_code,
                                 const std::vector<uint8_t>& response_buffer) {
  LOG(ERROR) << "\n\n====AUTHENTICATOR INFO RECEIVED====\n\n";

  state_ = State::kReady;
  supported_protocol_ = ProtocolVersion::kU2f;

  if (response_code == CTAPDeviceResponseCode::kSuccess) {
    auto device_info_response = ReadCTAPGetInfoResponse(response_buffer);
    if (device_info_response) {
      device_info_ = std::make_unique<AuthenticatorGetInfoResponse>(
          std::move(*device_info_response));
      supported_protocol_ = ProtocolVersion::kCtap;

      LOG(ERROR) << "\n\n\n==CTAP INFO RECEIVED AND WELL FORMED==\n\n\n";

      // Device supports CTAP 2.0, continue with device transaction
      DeviceTransact(std::move(command), std::move(callback));
      return;
    }
  }

  LOG(ERROR) << "\n\n FOUND U2F Device calling  u2f request\n\n";
  if (!u2f_callback_.is_null()) {
    std::move(u2f_callback_).Run(std::move(command), std::move(callback));
  }
}

bool FidoDevice::SupportsCTAP() const {
  return supported_protocol_ == ProtocolVersion::kCtap;
}

bool FidoDevice::AuthenticatorInfoObtained() const {
  return device_info_ == nullptr;
}

std::tuple<CTAPDeviceResponseCode, std::vector<uint8_t>>
FidoDevice::DecodeDeviceResponse(std::vector<uint8_t> buffer) {
  if (buffer.empty())
    return {CTAPDeviceResponseCode::kCtap2ErrInvalidCBOR,
            std::vector<uint8_t>()};

  if (supported_protocol_ == ProtocolVersion::kCtap) {
    auto response_code = GetResponseCode(buffer);
    buffer.erase(buffer.begin());
    return {response_code, buffer};
  }

  auto apdu_response = U2fApduResponse::CreateFromMessage(buffer);
  if (!apdu_response)
    return {CTAPDeviceResponseCode::kCtap1ErrOther, std::vector<uint8_t>()};

  switch (apdu_response->status()) {
    case U2fApduResponse::Status::SW_NO_ERROR:
      return {CTAPDeviceResponseCode::kSuccess, apdu_response->data()};
    case U2fApduResponse::Status::SW_CONDITIONS_NOT_SATISFIED:
      return {CTAPDeviceResponseCode::kCtap1ErrTimeout, std::vector<uint8_t>()};
    case U2fApduResponse::Status::SW_WRONG_LENGTH:
      return {CTAPDeviceResponseCode::kCtap1ErrInvalidLength,
              std::vector<uint8_t>()};
    case U2fApduResponse::Status::SW_WRONG_DATA:
      return {CTAPDeviceResponseCode::kCtap1ErrInvalidParameter,
              std::vector<uint8_t>()};
    default:
      break;
  }
  NOTREACHED();
  return {CTAPDeviceResponseCode::kCtap1ErrOther, std::vector<uint8_t>()};
}

CTAPDeviceResponseCode FidoDevice::GetDeviceProcessingError() {
  if (supported_protocol_ == ProtocolVersion::kCtap)
    return CTAPDeviceResponseCode::kCtap2ErrProcesssing;
  return CTAPDeviceResponseCode::kCtap1ErrOther;
}

}  // namespace device
