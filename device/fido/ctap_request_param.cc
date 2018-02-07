// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/ctap_request_param.h"

namespace device {

CtapRequestParam::CtapRequestParam() = default;
CtapRequestParam::~CtapRequestParam() = default;

bool CtapRequestParam::CheckU2fInteropCriteria() const {
  return false;
}

std::vector<uint8_t> CtapRequestParam::GetU2fApplicationParameter() const {
  return std::vector<uint8_t>();
}

std::vector<uint8_t> CtapRequestParam::GetU2fChallengeParameter() const {
  return std::vector<uint8_t>();
}

std::vector<std::vector<uint8_t>>
CtapRequestParam::GetU2fRegisteredKeysParameter() const {
  return std::vector<std::vector<uint8_t>>();
}

}  // namespace device
