// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_CTAP_MAKE_CREDENTIAL_H_
#define DEVICE_CTAP_CTAP_MAKE_CREDENTIAL_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/optional.h"
#include "device/ctap/authenticator_make_credential_response.h"
#include "device/ctap/ctap_constants.h"
#include "device/ctap/ctap_discovery.h"
#include "device/ctap/ctap_make_credential_request_param.h"
#include "device/ctap/ctap_request.h"
#include "device/ctap/fido_device.h"

namespace device {

class CTAPMakeCredential : public CTAPRequest {
 public:
  // Convert response buffer received from authenticator to
  // AuthenticatorMakeCredentialResponse object
  using MakeCredentialResponseCallback = base::OnceCallback<void(
      CTAPReturnCode return_code,
      base::Optional<AuthenticatorMakeCredentialResponse> response)>;

  static std::unique_ptr<CTAPRequest> MakeCredential(
      std::string relying_party_id,
      CTAPMakeCredentialRequestParam request_parameter,
      std::vector<CTAPDiscovery*> discoveries,
      MakeCredentialResponseCallback cb);

  CTAPMakeCredential(std::string relying_party_id,
                     CTAPMakeCredentialRequestParam request_parameter,
                     std::vector<CTAPDiscovery*> discoveries,
                     MakeCredentialResponseCallback cb);
  ~CTAPMakeCredential() override;

 private:
  void InitiateDeviceTransaction(FidoDevice* device) override;
  void DispatchU2FRequest(const std::string& device_id,
                          std::unique_ptr<CTAPRequestParam> command,
                          FidoDevice::DeviceCallback callback) override;
  void OnRequestResponseReceived(
      const std::string& device_id,
      CTAPDeviceResponseCode response_code,
      const std::vector<uint8_t>& response_data) override;
  base::Optional<AuthenticatorMakeCredentialResponse>
  CheckDeviceAndGetMakeCredentialResponse(
      const std::string& device_id,
      const std::vector<uint8_t>& response_data);

  CTAPMakeCredentialRequestParam request_parameter_;
  MakeCredentialResponseCallback cb_;

  base::WeakPtrFactory<CTAPMakeCredential> weak_factory_;
};

}  // namespace device

#endif  // DEVICE_CTAP_CTAP_MAKE_CREDENTIAL_H_
