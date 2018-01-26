// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_CTAP_GET_ASSERTION_H_
#define DEVICE_CTAP_CTAP_GET_ASSERTION_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/optional.h"
#include "device/ctap/authenticator_get_assertion_response.h"
#include "device/ctap/ctap_constants.h"
#include "device/ctap/ctap_discovery.h"
#include "device/ctap/ctap_get_assertion_request_param.h"
#include "device/ctap/ctap_request.h"
#include "device/ctap/fido_device.h"

namespace device {

class CTAPGetAssertion : public CTAPRequest {
 public:
  using GetAssertionResponseCallback = base::OnceCallback<void(
      CTAPReturnCode return_code,
      base::Optional<AuthenticatorGetAssertionResponse> response)>;

  static std::unique_ptr<CTAPRequest> GetAssertion(
      std::string relying_party_id,
      CTAPGetAssertionRequestParam request_parameter,
      std::vector<CTAPDiscovery*> discoveries,
      GetAssertionResponseCallback cb);

  CTAPGetAssertion(std::string relying_party_id,
                   CTAPGetAssertionRequestParam request_parameter,
                   std::vector<CTAPDiscovery*> discoveries,
                   GetAssertionResponseCallback cb);
  ~CTAPGetAssertion() override;

 private:
  void InitiateDeviceTransaction(FidoDevice* device) override;
  void DispatchU2FRequest(const std::string& device_id,
                          std::unique_ptr<CTAPRequestParam> command,
                          FidoDevice::DeviceCallback callback) override;
  void OnRequestResponseReceived(
      const std::string& device_id,
      CTAPDeviceResponseCode response_code,
      const std::vector<uint8_t>& response_data) override;
  base::Optional<AuthenticatorGetAssertionResponse>
  CheckDeviceAndGetGetAssertionResponse(const std::string& device_id,
                                        const std::vector<uint8_t>& reponse);

  CTAPGetAssertionRequestParam request_parameter_;
  GetAssertionResponseCallback cb_;

  base::WeakPtrFactory<CTAPGetAssertion> weak_factory_;
};

}  // namespace device

#endif  // DEVICE_CTAP_CTAP_GET_ASSERTION_H_
