// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/u2f_sign_param.h"

#include "device/u2f/u2f_apdu_command.h"

namespace device {

U2FSignParam::U2FSignParam(const std::vector<uint8_t>& app_parameter,
                           const std::vector<uint8_t>& challenge_digest,
                           const std::vector<uint8_t>& key_handle,
                           bool check_only)
    : app_parameter_(app_parameter),
      challenge_digest_(challenge_digest),
      key_handle_(key_handle),
      check_only_(check_only) {}

U2FSignParam::U2FSignParam(U2FSignParam&& that) = default;

U2FSignParam& U2FSignParam::operator=(U2FSignParam&& other) = default;

U2FSignParam::~U2FSignParam() = default;

base::Optional<std::vector<uint8_t>> U2FSignParam::Encode() const {
  auto sign_cmd = U2fApduCommand::CreateSign(app_parameter_, challenge_digest_,
                                             key_handle_, check_only_);

  if (!sign_cmd) {
    return base::nullopt;
  }
  return sign_cmd->GetEncodedCommand();
}

}  // namespace device
