// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/ctap_authenticator_request_param.h"

#include "base/numerics/safe_conversions.h"

namespace device {

// static
CTAPAuthenticatorRequestParam
CTAPAuthenticatorRequestParam::CreateGetInfoParam() {
  return CTAPAuthenticatorRequestParam(
      CtapRequestCommand::kAuthenticatorGetInfo);
}

// static
CTAPAuthenticatorRequestParam
CTAPAuthenticatorRequestParam::CreateGetNextAssertionParam() {
  return CTAPAuthenticatorRequestParam(
      CtapRequestCommand::kAuthenticatorGetNextAssertion);
}

// static
CTAPAuthenticatorRequestParam
CTAPAuthenticatorRequestParam::CreateResetParam() {
  return CTAPAuthenticatorRequestParam(CtapRequestCommand::kAuthenticatorReset);
}

// static
CTAPAuthenticatorRequestParam
CTAPAuthenticatorRequestParam::CreateCancelParam() {
  return CTAPAuthenticatorRequestParam(
      CtapRequestCommand::kAuthenticatorCancel);
}

CTAPAuthenticatorRequestParam::CTAPAuthenticatorRequestParam(
    CTAPAuthenticatorRequestParam&& that) = default;

CTAPAuthenticatorRequestParam& CTAPAuthenticatorRequestParam::operator=(
    CTAPAuthenticatorRequestParam&& that) = default;

CTAPAuthenticatorRequestParam::~CTAPAuthenticatorRequestParam() = default;

base::Optional<std::vector<uint8_t>> CTAPAuthenticatorRequestParam::Encode()
    const {
  return std::vector<uint8_t>{base::strict_cast<uint8_t>(cmd_)};
}

CTAPAuthenticatorRequestParam::CTAPAuthenticatorRequestParam(
    CtapRequestCommand cmd)
    : cmd_(cmd) {}

}  // namespace device
