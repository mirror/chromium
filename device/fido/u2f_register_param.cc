// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/u2f_register_param.h"

#include <memory>
#include <utility>

#include "components/apdu/apdu_command.h"
#include "device/fido/ctap_constants.h"

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

std::unique_ptr<apdu::ApduCommand>
U2fRegisterParam::CreateU2fRegisterApduCommand() const {
  if (app_id_digest_.size() != kAppIdDigestLen ||
      challenge_digest_.size() != kChallengeDigestLen) {
    return nullptr;
  }

  auto command = std::make_unique<apdu::ApduCommand>();
  std::vector<uint8_t> data(challenge_digest_.begin(), challenge_digest_.end());
  data.insert(data.end(), app_id_digest_.begin(), app_id_digest_.end());
  command->set_ins(kInsU2fEnroll);
  command->set_p1(kP1TupRequiredConsumed |
                  (is_individual_attestation_ ? kP1IndividualAttestation : 0));
  command->set_data(data);
  return command;
}

base::Optional<std::vector<uint8_t>> U2fRegisterParam::Encode() const {
  auto register_cmd = CreateU2fRegisterApduCommand();
  if (!register_cmd) {
    return base::nullopt;
  }
  return register_cmd->GetEncodedCommand();
}

}  // namespace device
