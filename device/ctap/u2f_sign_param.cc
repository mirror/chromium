// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/u2f_sign_param.h"

#include <utility>

#include "components/apdu/apdu_command.h"

namespace device {

U2FSignParam::U2FSignParam(std::vector<uint8_t> app_parameter,
                           std::vector<uint8_t> challenge_digest,
                           std::vector<uint8_t> key_handle)
    : app_parameter_(std::move(app_parameter)),
      challenge_digest_(std::move(challenge_digest)),
      key_handle_(std::move(key_handle)) {}

U2FSignParam::~U2FSignParam() = default;

base::Optional<std::vector<uint8_t>> U2FSignParam::Encode() const {
  auto sign_cmd = apdu::APDUCommand::CreateU2FSign(
      app_parameter_, challenge_digest_, key_handle_, check_only_);
  if (!sign_cmd) {
    return base::nullopt;
  }
  return sign_cmd->GetEncodedCommand();
}

U2FSignParam& U2FSignParam::SetCheckOnly(bool check_only) {
  check_only_ = check_only;
  return *this;
}

}  // namespace device
