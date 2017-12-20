// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_AUTHENTICATOR_MAKE_CREDENTIAL_RESPONSE_H_
#define DEVICE_CTAP_AUTHENTICATOR_MAKE_CREDENTIAL_RESPONSE_H_

#include <stdint.h>
#include <vector>

#include "device/ctap/ctap_response.h"
#include "device/ctap/ctap_response_code.h"

namespace device {

class AuthenticatorMakeCredentialResponse : public CTAPResponse {
 public:
  AuthenticatorMakeCredentialResponse(CTAPResponseCode response_code,
                                      std::vector<uint8_t> attestation_object);
  ~AuthenticatorMakeCredentialResponse() override;
  AuthenticatorMakeCredentialResponse(
      AuthenticatorMakeCredentialResponse&& that);

  const std::vector<uint8_t> get_attestation_object() const {
    return attestation_object_;
  };

 private:
  std::vector<uint8_t> attestation_object_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatorMakeCredentialResponse);
};

}  // namespace device

#endif  // DEVICE_CTAP_AUTHENTICATOR_MAKE_CREDENTIAL_RESPONSE_H_
