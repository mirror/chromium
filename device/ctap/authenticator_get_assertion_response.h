// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_CTAP_AUTHENTICATOR_GET_ASSERTION_RESPONSE_H_
#define DEVICE_CTAP_AUTHENTICATOR_GET_ASSERTION_RESPONSE_H_

#include <stdint.h>

#include <vector>

#include "base/macros.h"
#include "base/optional.h"
#include "device/ctap/ctap_authentication_response_data.h"
#include "device/ctap/public_key_credential_descriptor.h"
#include "device/ctap/public_key_credential_user_entity.h"

namespace device {

// Represents response from authenticators for AuthenticatorGetAssertion and
// AuthenticatorGetNextAssertion requests.
// https://fidoalliance.org/specs/fido-v2.0-rd-20170927/fido-client-to-authenticator-protocol-v2.0-rd-20170927.html#authenticatorGetAssertion
class AuthenticatorGetAssertionResponse
    : public CTAPAuthenticationResponseData {
 public:
  AuthenticatorGetAssertionResponse(PublicKeyCredentialDescriptor credential,
                                    std::vector<uint8_t> auth_data,
                                    std::vector<uint8_t> signature);
  AuthenticatorGetAssertionResponse(AuthenticatorGetAssertionResponse&& that);
  AuthenticatorGetAssertionResponse& operator=(
      AuthenticatorGetAssertionResponse&& other);
  ~AuthenticatorGetAssertionResponse();

  AuthenticatorGetAssertionResponse& SetUserEntity(
      PublicKeyCredentialUserEntity user);
  AuthenticatorGetAssertionResponse& SetNumCredentials(uint8_t num_credentials);

  const PublicKeyCredentialDescriptor& credential() const {
    return credential_;
  }
  const std::vector<uint8_t>& auth_data() const { return auth_data_; }
  const std::vector<uint8_t>& signature() const { return signature_; }
  const base::Optional<PublicKeyCredentialUserEntity>& user() const {
    return user_;
  }
  const base::Optional<uint8_t>& num_credentials() const {
    return num_credentials_;
  }

 private:
  PublicKeyCredentialDescriptor credential_;
  std::vector<uint8_t> auth_data_;
  std::vector<uint8_t> signature_;
  base::Optional<PublicKeyCredentialUserEntity> user_;
  base::Optional<uint8_t> num_credentials_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatorGetAssertionResponse);
};

}  // namespace device

#endif  // DEVICE_CTAP_AUTHENTICATOR_GET_ASSERTION_RESPONSE_H_
