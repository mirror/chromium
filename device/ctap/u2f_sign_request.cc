// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/u2f_sign_request.h"

#include <utility>

#include "base/bind.h"
#include "device/ctap/u2f_register_request.h"

namespace device {

// static
std::unique_ptr<U2FSignRequest> U2FSignRequest::TrySign(
    const std::string& relying_paty_id,
    std::unique_ptr<CTAPRequestParam> command,
    base::WeakPtr<FidoDevice> device,
    FidoDevice::DeviceCallback cb) {
  LOG(ERROR) << "U2F TRY SIGN";
  if (!command->CheckU2fInteropCriteria())
    return nullptr;

  std::unique_ptr<U2FSignRequest> request = std::make_unique<U2FSignRequest>(
      relying_paty_id, command->GetU2FRegisteredKeysParameter(),
      command->GetU2FApplicationParameter(),
      command->GetU2FChallengeParameter(), device, std::move(cb));

  if (request->device_) {
    if (!request->registered_keys_.empty()) {
      auto key_handle_iterator = request->registered_keys_.cbegin();
      request->DeviceSign(*key_handle_iterator, false,
                          base::BindOnce(&U2FSignRequest::OnTrySign,
                                         request->weak_factory_.GetWeakPtr(),
                                         key_handle_iterator));
    } else {
      NOTREACHED();
    }
  }
  return request;
}

U2FSignRequest::U2FSignRequest(
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

U2FSignRequest::~U2FSignRequest() = default;

std::vector<uint8_t> U2FSignRequest::GetResponseCredentialId() const {
  return response_credential_id_;
}

void U2FSignRequest::OnTrySign(
    std::vector<std::vector<uint8_t>>::const_iterator it,
    CTAPDeviceResponseCode return_code,
    const std::vector<uint8_t>& response_data) {
  LOG(ERROR) << "\n\n ON U2F TRY SIGN  \n\n";

  switch (return_code) {
    case CTAPDeviceResponseCode::kSuccess:
      if (it == registered_keys_.cend()) {
        std::move(cb_).Run(CTAPDeviceResponseCode::kCtap1ErrInvalidParameter,
                           std::vector<uint8_t>());
        return;
      }

      response_credential_id_ = *it;
      std::move(cb_).Run(CTAPDeviceResponseCode::kSuccess, response_data);
      return;

    case CTAPDeviceResponseCode::kCtap1ErrTimeout:
      // Waiting for user touch. Try again.
      // TODO(@hongjunchoi): Add delay.
      device_->Sign(std::make_unique<U2FSignParam>(application_parameter_,
                                                   challenge_parameter_, *it),
                    base::BindOnce(&U2FSignRequest::OnTrySign,
                                   weak_factory_.GetWeakPtr(), it));
      return;

    case CTAPDeviceResponseCode::kCtap1ErrInvalidParameter:
      // Key handle not accepted. Continue to iterate through the provided key
      // handles.
      if (++it != registered_keys_.end()) {
        DeviceSign(*it, false,
                   base::BindOnce(&U2FSignRequest::OnTrySign,
                                  weak_factory_.GetWeakPtr(), it));

      } else {
        // Authenticator does not accept any of the credential provided by the
        // relying party. Call bogus registration to obtain user presence.
        DeviceRegister(true, base::BindOnce(&U2FSignRequest::OnTrySign,
                                            weak_factory_.GetWeakPtr(),
                                            registered_keys_.cend()));
      }
      break;

    default:
      // Some sort of failure occurred. Abandon this device and move on.
      std::move(cb_).Run(CTAPDeviceResponseCode::kCtap1ErrOther,
                         std::vector<uint8_t>());
      break;
  }
}

}  // namespace device
