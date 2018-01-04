// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/authenticator_get_info_response.h"

#include <utility>

namespace device {

AuthenticatorGetInfoResponse::AuthenticatorGetInfoResponse(
    constants::CTAPDeviceResponseCode response_code,
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
    std::vector<std::string> extensions) {
  extensions_ = std::move(extensions);
}

void AuthenticatorGetInfoResponse::SetMaxMsgSize(
    std::vector<uint8_t> max_msg_size) {
  max_msg_size_ = std::move(max_msg_size);
}

void AuthenticatorGetInfoResponse::SetPinProtocols(
    std::vector<uint8_t> pin_protocols) {
  pin_protocols_ = std::move(pin_protocols);
}

void AuthenticatorGetInfoResponse::SetOptions(AuthenticatorOptions options) {
  options_ = std::move(options);
}

}  // namespace device
