// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/u2f_register_request.h"

#include <utility>

#include "base/bind.h"

namespace device {

// static
std::unique_ptr<U2FRegisterRequest> U2FRegisterRequest::TryRegistration(
    const std::string& relying_party_id,
    std::unique_ptr<CTAPRequestParam> command,
    base::WeakPtr<FidoDevice> device,
    FidoDevice::DeviceCallback cb) {
  if (!command->CheckU2fInteropCriteria())
    return nullptr;

  std::unique_ptr<U2FRegisterRequest> request =
      std::make_unique<U2FRegisterRequest>(
          relying_party_id, command->GetU2FRegisteredKeysParameter(),
          command->GetU2FApplicationParameter(),
          command->GetU2FChallengeParameter(), device, std::move(cb));

  if (request->device_) {
    if (!request->registered_keys_.empty()) {
      auto key_handle_iterator = request->registered_keys_.cbegin();
      request->DeviceSign(
          *key_handle_iterator, true,
          base::BindOnce(&U2FRegisterRequest::OnTryCheckDuplicateRegistration,
                         request->weak_factory_.GetWeakPtr(),
                         key_handle_iterator));

    } else {
      request->DeviceRegister(
          false, base::BindOnce(&U2FRegisterRequest::OnTryRegistration,
                                request->weak_factory_.GetWeakPtr(), false));
    }
  }
  return request;
}

U2FRegisterRequest::U2FRegisterRequest(
    const std::string& relying_paty_id,
    std::vector<std::vector<uint8_t>> registered_keys,
    std::vector<uint8_t> application_parameter,
    std::vector<uint8_t> challenge_parameter,
    base::WeakPtr<FidoDevice> device,
    FidoDevice::DeviceCallback cb)
    : U2FDeviceRequest(relying_paty_id,
                       registered_keys,
                       application_parameter,
                       challenge_parameter,
                       device,
                       std::move(cb)),
      weak_factory_(this) {}

U2FRegisterRequest::~U2FRegisterRequest() = default;

void U2FRegisterRequest::OnTryCheckDuplicateRegistration(
    std::vector<std::vector<uint8_t>>::const_iterator it,
    CTAPDeviceResponseCode return_code,
    const std::vector<uint8_t>& response_data) {
  switch (return_code) {
    case CTAPDeviceResponseCode::kSuccess:
    case CTAPDeviceResponseCode::kCtap1ErrTimeout:
      // Duplicate registration found. Call bogus registration to check for
      // user presence (touch) and terminate the registration process.
      DeviceRegister(true,
                     base::BindOnce(&U2FRegisterRequest::OnTryRegistration,
                                    weak_factory_.GetWeakPtr(), true));
      break;

    case CTAPDeviceResponseCode::kCtap1ErrInvalidParameter:
      // Continue to iterate through the provided key handles in the exclude
      // list and check for already registered keys.
      if (++it != registered_keys_.end()) {
        DeviceSign(
            *it, true,
            base::BindOnce(&U2FRegisterRequest::OnTryCheckDuplicateRegistration,
                           weak_factory_.GetWeakPtr(), it));
      } else {
        // All key handles in exclude list has been checked, proceed with
        // registration.
        DeviceRegister(false,
                       base::BindOnce(&U2FRegisterRequest::OnTryRegistration,
                                      weak_factory_.GetWeakPtr(), false));
      }
      break;
    default:
      // Some sort of failure occurred. Abandon this device and move on.
      std::move(cb_).Run(CTAPDeviceResponseCode::kCtap1ErrOther,
                         std::vector<uint8_t>());
      break;
  }
}

void U2FRegisterRequest::OnTryRegistration(
    bool is_duplicate_registration,
    CTAPDeviceResponseCode return_code,
    const std::vector<uint8_t>& response_data) {
  switch (return_code) {
    case CTAPDeviceResponseCode::kSuccess: {
      LOG(ERROR) << "ON TRY U2F REGISTRATION SUCCESS";

      if (is_duplicate_registration) {
        std::move(cb_).Run(CTAPDeviceResponseCode::kCtap1ErrInvalidParameter,
                           std::vector<uint8_t>());
        return;
      }

      // TODO(@hongjunchoi): Convert |response_data| into attestation object as
      // specified in section 7.1.4 of the CTAP spec.
      std::move(cb_).Run(CTAPDeviceResponseCode::kSuccess, response_data);
      return;
    }

    case CTAPDeviceResponseCode::kCtap1ErrTimeout: {
      // LOG(ERROR) << "ON TRY U2F REGISTRATION  WAITING USER TOUCH TIMEOUT";
      // Awaiting user touch retry.
      // TODO(@hongjunchoi): Add delay.
      DeviceRegister(false,
                     base::BindOnce(&U2FRegisterRequest::OnTryRegistration,
                                    weak_factory_.GetWeakPtr(), false));
      break;
    }

    default:
      LOG(ERROR) << "\n\n\n ON TRY U2F REGISTRATION ERROR\n\n\n";

      // Some sort of failure occurred. Abandon this device and move on.
      std::move(cb_).Run(CTAPDeviceResponseCode::kCtap1ErrOther, response_data);
      break;
  }
  return;
}

}  // namespace device
