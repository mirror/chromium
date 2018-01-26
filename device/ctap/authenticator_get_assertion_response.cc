// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/authenticator_get_assertion_response.h"

#include <utility>

namespace device {

AuthenticatorGetAssertionResponse::AuthenticatorGetAssertionResponse(
    PublicKeyCredentialDescriptor credential,
    std::vector<uint8_t> auth_data,
    std::vector<uint8_t> signature)
    : CTAPAuthenticationResponseData(credential.id()),
      credential_(credential),
      auth_data_(std::move(auth_data)),
      signature_(std::move(signature)) {}

AuthenticatorGetAssertionResponse::AuthenticatorGetAssertionResponse(
    AuthenticatorGetAssertionResponse&& that) = default;

AuthenticatorGetAssertionResponse& AuthenticatorGetAssertionResponse::operator=(
    AuthenticatorGetAssertionResponse&& other) = default;

AuthenticatorGetAssertionResponse::~AuthenticatorGetAssertionResponse() =
    default;

AuthenticatorGetAssertionResponse&
AuthenticatorGetAssertionResponse::SetUserEntity(
    PublicKeyCredentialUserEntity user) {
  user_ = std::move(user);
  return *this;
}

AuthenticatorGetAssertionResponse&
AuthenticatorGetAssertionResponse::SetNumCredentials(uint8_t num_credentials) {
  num_credentials_ = num_credentials;
  return *this;
}

}  // namespace device
