// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/ctap_authenticator_request_param.h"

#include <utility>

#include "base/numerics/safe_conversions.h"

namespace device {

// static
CTAPAuthenticatorRequestParam
CTAPAuthenticatorRequestParam::CreateAuthenticatorGetInfoParam() {
  return CTAPAuthenticatorRequestParam(
      constants::CTAPRequestCommand::kAuthenticatorGetInfo);
}

// static
CTAPAuthenticatorRequestParam
CTAPAuthenticatorRequestParam::CreateAuthenticatorGetNextAssertionParam() {
  return CTAPAuthenticatorRequestParam(
      constants::CTAPRequestCommand::kAuthenticatorGetNextAssertion);
}

// static
CTAPAuthenticatorRequestParam
CTAPAuthenticatorRequestParam::CreateAuthenticatorResetParam() {
  return CTAPAuthenticatorRequestParam(
      constants::CTAPRequestCommand::kAuthenticatorReset);
}

// static
CTAPAuthenticatorRequestParam
CTAPAuthenticatorRequestParam::CreateAuthenticatorCancelParam() {
  return CTAPAuthenticatorRequestParam(
      constants::CTAPRequestCommand::kAuthenticatorCancel);
}

CTAPAuthenticatorRequestParam::CTAPAuthenticatorRequestParam(
    constants::CTAPRequestCommand cmd)
    : cmd_(cmd) {}

CTAPAuthenticatorRequestParam::CTAPAuthenticatorRequestParam(
    CTAPAuthenticatorRequestParam&& that) = default;

CTAPAuthenticatorRequestParam& CTAPAuthenticatorRequestParam::operator=(
    CTAPAuthenticatorRequestParam&& that) = default;

CTAPAuthenticatorRequestParam::~CTAPAuthenticatorRequestParam() = default;

base::Optional<std::vector<uint8_t>>
CTAPAuthenticatorRequestParam::SerializeToCBOR() const {
  return std::vector<uint8_t>{base::strict_cast<uint8_t>(cmd_)};
}

}  // namespace device
