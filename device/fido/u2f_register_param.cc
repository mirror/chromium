// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/u2f_register_param.h"

#include <utility>

#include "device/fido/u2f_apdu_command.h"

namespace device {

U2fRegisterParam::U2fRegisterParam(std::vector<uint8_t> app_id_digest,
                                   std::vector<uint8_t> challenge_digest,
                                   bool is_individual_attestation)
    : app_id_digest_(std::move(app_id_digest)),
      challenge_digest_(std::move(challenge_digest)),
      is_individual_attestation_(is_individual_attestation) {}
U2fRegisterParam::U2fRegisterParam(U2fRegisterParam&& that) = default;
U2fRegisterParam& U2fRegisterParam::operator=(U2fRegisterParam&& that) =
    default;
U2fRegisterParam::~U2fRegisterParam() = default;

base::Optional<std::vector<uint8_t>> U2fRegisterParam::Encode() const {
  auto register_cmd = U2fApduCommand::CreateRegister(
      app_id_digest_, challenge_digest_, is_individual_attestation_);
  if (!register_cmd) {
    return base::nullopt;
  }
  return register_cmd->GetEncodedCommand();
}

}  // namespace device
