// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/ctap_authenticator_response.h"

namespace device {

CTAPAuthenticatorResponse::CTAPAuthenticatorResponse(
    constants::CTAPDeviceResponseCode response_code)
    : CTAPResponse(response_code) {}

CTAPAuthenticatorResponse::CTAPAuthenticatorResponse(
    CTAPAuthenticatorResponse&& that) = default;

CTAPAuthenticatorResponse& CTAPAuthenticatorResponse::operator=(
    CTAPAuthenticatorResponse&& other) = default;

CTAPAuthenticatorResponse::~CTAPAuthenticatorResponse() = default;

}  // namespace device
