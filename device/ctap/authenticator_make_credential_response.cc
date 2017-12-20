// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/authenticator_make_credential_response.h"

namespace device {

AuthenticatorMakeCredentialResponse::AuthenticatorMakeCredentialResponse(
    CTAPResponseCode response_code,
    std::vector<uint8_t> attestation_object)
    : CTAPResponse(response_code),
      attestation_object_(std::move(attestation_object)){};

AuthenticatorMakeCredentialResponse::~AuthenticatorMakeCredentialResponse() =
    default;

AuthenticatorMakeCredentialResponse::AuthenticatorMakeCredentialResponse(
    AuthenticatorMakeCredentialResponse&& that) = default;

}  // namespace device
