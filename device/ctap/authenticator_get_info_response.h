// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_AUTHENTICATOR_GET_INFO_RESPONSE_H_
#define DEVICE_CTAP_AUTHENTICATOR_GET_INFO_RESPONSE_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "base/optional.h"
#include "device/ctap/authenticator_options.h"
#include "device/ctap/ctap_constants.h"
#include "device/ctap/ctap_response.h"

namespace device {

// Authenticator response for AuthenticatorGetInfo request that encapsulates
// versions, options, aaguid, other authenticator device information.
// https://fidoalliance.org/specs/fido-v2.0-rd-20170927/fido-client-to-authenticator-protocol-v2.0-rd-20170927.html#authenticatorGetInfo
class AuthenticatorGetInfoResponse : public CTAPResponse {
 public:
  AuthenticatorGetInfoResponse(constants::CTAPDeviceResponseCode response_code,
                               std::vector<std::string> versions,
                               std::vector<uint8_t> aaguid);
  AuthenticatorGetInfoResponse(AuthenticatorGetInfoResponse&& that);
  AuthenticatorGetInfoResponse& operator=(AuthenticatorGetInfoResponse&& other);
  ~AuthenticatorGetInfoResponse() override;

  void SetExtensions(std::vector<std::string> extensions);
  void SetMaxMsgSize(std::vector<uint8_t> max_msg_size);
  void SetPinProtocols(std::vector<uint8_t> pin_protocols);
  void SetOptions(AuthenticatorOptions options);

  const std::vector<std::string>& versions() { return versions_; }
  const base::Optional<std::vector<std::string>>& extensions() const {
    return extensions_;
  }
  const std::vector<uint8_t>& aaguid() const { return aaguid_; }
  const base::Optional<std::vector<uint8_t>>& max_msg_size() const {
    return max_msg_size_;
  }
  const base::Optional<std::vector<uint8_t>>& pin_protocol() const {
    return pin_protocols_;
  }

 private:
  std::vector<std::string> versions_;
  base::Optional<std::vector<std::string>> extensions_;
  std::vector<uint8_t> aaguid_;
  base::Optional<std::vector<uint8_t>> max_msg_size_;
  base::Optional<std::vector<uint8_t>> pin_protocols_;
  base::Optional<AuthenticatorOptions> options_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatorGetInfoResponse);
};

}  // namespace device

#endif  // DEVICE_CTAP_AUTHENTICATOR_GET_INFO_RESPONSE_H_
