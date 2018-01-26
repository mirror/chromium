// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_U2F_REGISTER_REQUEST_H_
#define DEVICE_CTAP_U2F_REGISTER_REQUEST_H_

#include <memory>
#include <string>
#include <vector>

#include "device/ctap/ctap_constants.h"
#include "device/ctap/fido_device.h"
#include "device/ctap/u2f_device_request.h"

namespace device {

class U2FRegisterRequest : public U2FDeviceRequest {
 public:
  static std::unique_ptr<U2FRegisterRequest> TryRegistration(
      const std::string& relying_party_id,
      std::unique_ptr<CTAPRequestParam> command,
      base::WeakPtr<FidoDevice> device,
      FidoDevice::DeviceCallback cb);

  U2FRegisterRequest(const std::string& relying_party_id,
                     std::vector<std::vector<uint8_t>> registered_keys,
                     std::vector<uint8_t> application_parameter,
                     std::vector<uint8_t> challenge_parameter,
                     base::WeakPtr<FidoDevice> device,
                     FidoDevice::DeviceCallback cb);
  ~U2FRegisterRequest() override;

 private:
  // Callback function called when non-empty exclude list was provided. This
  // function iterates through all key handles in |registered_keys_| and checks
  // for already registered keys.
  void OnTryCheckDuplicateRegistration(
      std::vector<std::vector<uint8_t>>::const_iterator it,
      CTAPDeviceResponseCode return_code,
      const std::vector<uint8_t>& response_data);

  // Callback function called when empty exclude list was provided or when all
  // key handles in exclude has been checked for duplicate registration.
  void OnTryRegistration(bool is_duplicate_registration,
                         CTAPDeviceResponseCode return_code,
                         const std::vector<uint8_t>& response_data);

  base::WeakPtrFactory<U2FRegisterRequest> weak_factory_;
};

}  // namespace device

#endif  // DEVICE_CTAP_U2F_REGISTER_REQUEST_H_
