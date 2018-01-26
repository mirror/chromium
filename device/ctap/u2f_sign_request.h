// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_U2F_SIGN_REQUEST_H_
#define DEVICE_CTAP_U2F_SIGN_REQUEST_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/optional.h"
#include "device/ctap/ctap_constants.h"
#include "device/ctap/fido_device.h"
#include "device/ctap/u2f_device_request.h"

namespace device {

class U2FSignRequest : public U2FDeviceRequest {
 public:
  static std::unique_ptr<U2FSignRequest> TrySign(
      const std::string& relying_party_id,
      std::unique_ptr<CTAPRequestParam> command,
      base::WeakPtr<FidoDevice> device,
      FidoDevice::DeviceCallback cb);

  U2FSignRequest(const std::string& relying_party_id,
                 std::vector<std::vector<uint8_t>> registered_keys,
                 std::vector<uint8_t> application_parameter,
                 std::vector<uint8_t> challenge_parameter,
                 base::WeakPtr<FidoDevice> device,
                 FidoDevice::DeviceCallback cb);
  ~U2FSignRequest() override;

  std::vector<uint8_t> GetResponseCredentialId() const override;

 private:
  // Callback to U2F Sign operation. Loops through provided |registered_keys|
  // and signs the key handle if it is accepted by the device. If no key handles
  // in |registered_keys| are accepted by the authenticator, invoke
  // TryRegistration() with bogus parameters to obtain user presence and
  // terminate the operation.
  void OnTrySign(std::vector<std::vector<uint8_t>>::const_iterator it,
                 CTAPDeviceResponseCode return_code,
                 const std::vector<uint8_t>& response_data);

  std::vector<uint8_t> response_credential_id_;
  base::WeakPtrFactory<U2FSignRequest> weak_factory_;
};

}  // namespace device

#endif  // DEVICE_CTAP_U2F_SIGN_REQUEST_H_
