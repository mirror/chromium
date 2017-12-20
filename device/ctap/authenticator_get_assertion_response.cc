// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/authenticator_get_assertion_response.h"

namespace device {

AuthenticatorGetAssertionResponse::AuthenticatorGetAssertionResponse(
    CTAPResponseCode response_code,
    std::vector<uint8_t> auth_data,
    std::vector<uint8_t> signature,
    PublicKeyCredentialUserEntity user)
    : CTAPResponse(response_code),
      auth_data_(std::move(auth_data)),
      signature_(std::move(signature)),
      user_(std::move(user)){};

AuthenticatorGetAssertionResponse::~AuthenticatorGetAssertionResponse() =
    default;

AuthenticatorGetAssertionResponse::AuthenticatorGetAssertionResponse(
    AuthenticatorGetAssertionResponse&& that) = default;

base::Optional<PublicKeyCredentialDescriptor>
AuthenticatorGetAssertionResponse::get_credential() const {
  if (credential_)
    return credential_.value().Clone();
  return base::nullopt;
}

void AuthenticatorGetAssertionResponse::set_credential(
    PublicKeyCredentialDescriptor& credential) {
  credential_ = std::move(credential);
}

void AuthenticatorGetAssertionResponse::set_num_credentials(
    const uint8_t& num_credentials) {
  num_credentials_ = num_credentials;
}

}  // namespace device
