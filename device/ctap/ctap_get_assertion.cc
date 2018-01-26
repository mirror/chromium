// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/ctap_get_assertion.h"

#include <memory>
#include <utility>

#include "device/ctap/ctap_discovery.h"
#include "device/ctap/device_response_converter.h"
#include "device/ctap/u2f_sign_request.h"

namespace device {

// static
std::unique_ptr<CTAPRequest> CTAPGetAssertion::GetAssertion(
    std::string relying_party_id,
    CTAPGetAssertionRequestParam request_parameter,
    std::vector<CTAPDiscovery*> discoveries,
    GetAssertionResponseCallback cb) {
  std::unique_ptr<CTAPRequest> request = std::make_unique<CTAPGetAssertion>(
      std::move(relying_party_id), std::move(request_parameter),
      std::move(discoveries), std::move(cb));
  LOG(ERROR) << "\n\n====GET ASSERTION REQUEST START====\n\n";
  request->Start();
  return request;
}

CTAPGetAssertion::CTAPGetAssertion(
    std::string relying_party_id,
    CTAPGetAssertionRequestParam request_parameter,
    std::vector<CTAPDiscovery*> discoveries,
    GetAssertionResponseCallback cb)
    : CTAPRequest(std::move(relying_party_id), std::move(discoveries)),
      request_parameter_(std::move(request_parameter)),
      cb_(std::move(cb)),
      weak_factory_(this) {}

CTAPGetAssertion::~CTAPGetAssertion() = default;

void CTAPGetAssertion::InitiateDeviceTransaction(FidoDevice* device) {
  LOG(ERROR) << "\n\n====STARTING GET ASSERTION FOR DEVICE===\n\n";
  device->GetAssertion(
      std::make_unique<CTAPGetAssertionRequestParam>(request_parameter_),
      base::BindOnce(&CTAPGetAssertion::OnRequestResponseReceived,
                     weak_factory_.GetWeakPtr(), device->GetId()),
      base::BindOnce(&CTAPGetAssertion::DispatchU2FRequest,
                     weak_factory_.GetWeakPtr(), device->GetId()));
}

void CTAPGetAssertion::DispatchU2FRequest(
    const std::string& device_id,
    std::unique_ptr<CTAPRequestParam> command,
    FidoDevice::DeviceCallback callback) {
  LOG(ERROR) << "\n\n\n DISPATHCING U2F sign register \n\n\n";
  base::AutoLock device_lock(device_list_lock_);
  base::AutoLock u2f_request_lock(u2f_request_tracker_lock_);
  if (CanContinueDeviceTransaction() && devices_.count(device_id) &&
      !child_u2f_requests_.count(device_id)) {
    child_u2f_requests_[device_id] = U2FSignRequest::TrySign(
        relying_party_id_, std::move(command),
        devices_[device_id]->GetWeakPtr(), std::move(callback));
  }
}

void CTAPGetAssertion::OnRequestResponseReceived(
    const std::string& device_id,
    CTAPDeviceResponseCode response_code,
    const std::vector<uint8_t>& response_data) {
  LOG(ERROR) << "\n\n\n==FINALY GET ASSERTION RESPONSE RECEIVED==\n\n\n";
  LOG(ERROR) << response_data.data();

  auto response_object =
      CheckDeviceAndGetGetAssertionResponse(device_id, response_data);
  if (!response_object)
    return;

  LOG(ERROR) << "\n\nRESPONSE TO GET ASSERTIONj WELL FORMED\n\n";
  switch (response_code) {
    case CTAPDeviceResponseCode::kSuccess:
      LOG(ERROR) << "\n\n\n  ====  DEVICE RESPONSE SUCCESS  ==== \n\n\n";
      state_.store(State::kComplete);
      device_list_lock_.Acquire();
      u2f_request_tracker_lock_.Acquire();

      for (const auto device_it : devices_) {
        if (device_it.first != device_id) {
          CancelDeviceTransaction(device_it.second);
          devices_.erase(device_it.first);
          child_u2f_requests_.erase(device_it.first);
        }
      }

      device_list_lock_.Release();
      u2f_request_tracker_lock_.Release();

      std::move(cb_).Run(CTAPReturnCode::kSuccess, std::move(response_object));
      break;

    default:
      LOG(ERROR) << "\n\n\n  ====DEVICE RESPONSE ERROR====\n\n\n";
      base::AutoLock device_lock(device_list_lock_);
      base::AutoLock u2f_lock(u2f_request_tracker_lock_);
      devices_.erase(device_id);
      child_u2f_requests_.erase(device_id);
      break;
  }
  return;
}

base::Optional<AuthenticatorGetAssertionResponse>
CTAPGetAssertion::CheckDeviceAndGetGetAssertionResponse(
    const std::string& device_id,
    const std::vector<uint8_t>& reponse) {
  base::AutoLock device_list_lock(device_list_lock_);
  base::AutoLock u2f_request_lock(u2f_request_tracker_lock_);

  auto device_it = devices_.find(device_id);
  auto request_it = child_u2f_requests_.find(device_id);

  if (device_it == devices_.end() ||
      (!device_it->second->SupportsCTAP() &&
       request_it == child_u2f_requests_.end())) {
    devices_.erase(device_id);
    child_u2f_requests_.erase(device_id);
    return base::nullopt;
  }

  auto* device = device_it->second;
  auto response_object =
      device->SupportsCTAP()
          ? ReadCTAPGetAssertionResponse(reponse,
                                         request_parameter_.allow_list())
          : ReadU2FSignResponse(relying_party_id_, reponse,
                                request_it->second->GetResponseCredentialId());

  if (!response_object) {
    LOG(ERROR) << "\n\n\n RESPONSE TO GET ASSERTION NOT WELL FORMED \n\n\n";
    devices_.erase(device_id);
    child_u2f_requests_.erase(device_id);
    return base::nullopt;
  }

  return response_object;
}

}  // namespace device
