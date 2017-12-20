// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_AUTHENTICATOR_GET_ASSERTION_RESPONSE_H_
#define DEVICE_CTAP_AUTHENTICATOR_GET_ASSERTION_RESPONSE_H_

#include <stdint.h>
#include <vector>

#include "base/optional.h"
#include "device/ctap/ctap_response.h"
#include "device/ctap/public_key_credential_descriptor.h"
#include "device/ctap/public_key_credential_user_entity.h"

namespace device {

class AuthenticatorGetAssertionResponse : public CTAPResponse {
 public:
  AuthenticatorGetAssertionResponse(CTAPResponseCode response_code,
                                    std::vector<uint8_t> auth_data,
                                    std::vector<uint8_t> signature,
                                    PublicKeyCredentialUserEntity user);
  ~AuthenticatorGetAssertionResponse() override;
  AuthenticatorGetAssertionResponse(AuthenticatorGetAssertionResponse&& that);

  base::Optional<PublicKeyCredentialDescriptor> get_credential() const;
  const std::vector<uint8_t> get_auth_data() const { return auth_data_; };
  const std::vector<uint8_t> get_signature() const { return signature_; };
  const PublicKeyCredentialUserEntity get_user() const {
    return user_.Clone();
  };
  const base::Optional<uint8_t> get_num_credentials() const {
    return num_credentials_;
  };
  void set_credential(PublicKeyCredentialDescriptor& credential);
  void set_num_credentials(const uint8_t& num_credentials);

 private:
  base::Optional<PublicKeyCredentialDescriptor> credential_;
  std::vector<uint8_t> auth_data_;
  std::vector<uint8_t> signature_;
  PublicKeyCredentialUserEntity user_;
  base::Optional<uint8_t> num_credentials_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatorGetAssertionResponse);
};

}  // namespace device

#endif  // DEVICE_CTAP_AUTHENTICATOR_GET_ASSERTION_RESPONSE_H_
