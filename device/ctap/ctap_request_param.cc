// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/ctap_request_param.h"

namespace device {

CTAPRequestParam::CTAPRequestParam() = default;
CTAPRequestParam::~CTAPRequestParam() = default;

bool CTAPRequestParam::CheckU2fInteropCriteria() const {
  return false;
}

std::vector<uint8_t> CTAPRequestParam::GetU2FApplicationParameter() const {
  return std::vector<uint8_t>();
}

std::vector<uint8_t> CTAPRequestParam::GetU2FChallengeParameter() const {
  return std::vector<uint8_t>();
}

std::vector<std::vector<uint8_t>>
CTAPRequestParam::GetU2FRegisteredKeysParameter() const {
  return std::vector<std::vector<uint8_t>>();
}

}  // namespace device
