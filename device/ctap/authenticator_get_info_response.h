// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_AUTHENTICATOR_GET_INFO_RESPONSE_H_
#define DEVICE_CTAP_AUTHENTICATOR_GET_INFO_RESPONSE_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "base/optional.h"
#include "device/ctap/ctap_constants.h"
#include "device/ctap/ctap_response.h"

namespace device {

class AuthenticatorGetInfoResponse : public CTAPResponse {
 public:
  AuthenticatorGetInfoResponse(constants::CTAPResponseCode response_code,
                               std::vector<std::string> versions,
                               std::vector<uint8_t> aaguid);
  AuthenticatorGetInfoResponse(AuthenticatorGetInfoResponse&& that);
  AuthenticatorGetInfoResponse& operator=(AuthenticatorGetInfoResponse&& other);
  ~AuthenticatorGetInfoResponse() override;

  const std::vector<std::string>& get_versions() { return versions_; }
  base::Optional<std::vector<std::string>> get_extensions() const {
    return extensions_;
  }
  const std::vector<uint8_t>& get_aaguid() const { return aaguid_; }
  base::Optional<std::vector<uint8_t>> GetMaxMsgSize() const {
    return max_msg_size_;
  }
  const base::Optional<std::vector<uint8_t>>& get_pin_protocol() const {
    return pin_protocols_;
  }
  void SetExtensions(const std::vector<std::string>& extensions);
  void SetMaxMsg_size(const std::vector<uint8_t>& max_msg_size);
  void SetPinProtocols(const std::vector<uint8_t>& pin_protocols);

 private:
  std::vector<std::string> versions_;
  base::Optional<std::vector<std::string>> extensions_;
  std::vector<uint8_t> aaguid_;
  base::Optional<std::vector<uint8_t>> max_msg_size_;
  base::Optional<std::vector<uint8_t>> pin_protocols_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatorGetInfoResponse);
};

}  // namespace device

#endif  // DEVICE_CTAP_AUTHENTICATOR_GET_INFO_RESPONSE_H_
