// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/u2f_register_param.h"

#include <utility>

#include "components/apdu/apdu_command.h"

namespace device {

U2FRegisterParam::U2FRegisterParam(std::vector<uint8_t> app_id_digest,
                                   std::vector<uint8_t> challenge_digest,
                                   bool is_individual_attestation)
    : app_id_digest_(std::move(app_id_digest)),
      challenge_digest_(std::move(challenge_digest)),
      is_individual_attestation_(is_individual_attestation) {}

U2FRegisterParam::~U2FRegisterParam() = default;

base::Optional<std::vector<uint8_t>> U2FRegisterParam::Encode() const {
  auto register_cmd = apdu::APDUCommand::CreateU2FRegister(
      app_id_digest_, challenge_digest_, is_individual_attestation_);
  if (!register_cmd) {
    return base::nullopt;
  }
  return register_cmd->GetEncodedCommand();
}

}  // namespace device
