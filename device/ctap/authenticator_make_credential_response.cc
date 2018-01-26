// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/authenticator_make_credential_response.h"

#include <utility>

namespace device {

AuthenticatorMakeCredentialResponse::AuthenticatorMakeCredentialResponse(
    std::vector<uint8_t> credential_id,
    std::vector<uint8_t> attestation_object)
    : CTAPAuthenticationResponseData(std::move(credential_id)),
      attestation_object_(std::move(attestation_object)) {}

AuthenticatorMakeCredentialResponse::AuthenticatorMakeCredentialResponse(
    AuthenticatorMakeCredentialResponse&& that) = default;

AuthenticatorMakeCredentialResponse& AuthenticatorMakeCredentialResponse::
operator=(AuthenticatorMakeCredentialResponse&& other) = default;

AuthenticatorMakeCredentialResponse::~AuthenticatorMakeCredentialResponse() =
    default;

}  // namespace device
