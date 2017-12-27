// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/authenticator_get_info_response.h"

#include <utility>

namespace device {

AuthenticatorGetInfoResponse::AuthenticatorGetInfoResponse(
    constants::CTAPResponseCode response_code,
    std::vector<std::string> versions,
    std::vector<uint8_t> aaguid)
    : CTAPResponse(response_code),
      versions_(std::move(versions)),
      aaguid_(std::move(aaguid)) {}

AuthenticatorGetInfoResponse::AuthenticatorGetInfoResponse(
    AuthenticatorGetInfoResponse&& that) = default;

AuthenticatorGetInfoResponse& AuthenticatorGetInfoResponse::operator=(
    AuthenticatorGetInfoResponse&& other) = default;

AuthenticatorGetInfoResponse::~AuthenticatorGetInfoResponse() = default;

void AuthenticatorGetInfoResponse::SetExtensions(
    const std::vector<std::string>& extensions) {
  extensions_ = extensions;
}

void AuthenticatorGetInfoResponse::SetMaxMsgSize(
    const std::vector<uint8_t>& max_msg_size) {
  max_msg_size_ = max_msg_size;
}

void AuthenticatorGetInfoResponse::SetPinProtocols(
    const std::vector<uint8_t>& pin_protocols) {
  pin_protocols_ = pin_protocols;
}

}  // namespace device
