// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/ctap_authenticator_request_param.h"

#include "base/numerics/safe_conversions.h"

namespace device {

// static
CtapAuthenticatorRequestParam
CtapAuthenticatorRequestParam::CreateGetInfoParam() {
  return CtapAuthenticatorRequestParam(
      CtapRequestCommand::kAuthenticatorGetInfo);
}

// static
CtapAuthenticatorRequestParam
CtapAuthenticatorRequestParam::CreateGetNextAssertionParam() {
  return CtapAuthenticatorRequestParam(
      CtapRequestCommand::kAuthenticatorGetNextAssertion);
}

// static
CtapAuthenticatorRequestParam
CtapAuthenticatorRequestParam::CreateResetParam() {
  return CtapAuthenticatorRequestParam(CtapRequestCommand::kAuthenticatorReset);
}

// static
CtapAuthenticatorRequestParam
CtapAuthenticatorRequestParam::CreateCancelParam() {
  return CtapAuthenticatorRequestParam(
      CtapRequestCommand::kAuthenticatorCancel);
}

CtapAuthenticatorRequestParam::CtapAuthenticatorRequestParam(
    CtapAuthenticatorRequestParam&& that) = default;

CtapAuthenticatorRequestParam& CtapAuthenticatorRequestParam::operator=(
    CtapAuthenticatorRequestParam&& that) = default;

CtapAuthenticatorRequestParam::~CtapAuthenticatorRequestParam() = default;

base::Optional<std::vector<uint8_t>> CtapAuthenticatorRequestParam::Encode()
    const {
  return std::vector<uint8_t>{base::strict_cast<uint8_t>(cmd_)};
}

CtapAuthenticatorRequestParam::CtapAuthenticatorRequestParam(
    CtapRequestCommand cmd)
    : cmd_(cmd) {}

}  // namespace device
