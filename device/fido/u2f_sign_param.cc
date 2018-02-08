// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/u2f_sign_param.h"

#include <memory>
#include <utility>

#include "components/apdu/apdu_command.h"
#include "device/fido/ctap_constants.h"

namespace device {

U2fSignParam::U2fSignParam(std::vector<uint8_t> app_parameter,
                           std::vector<uint8_t> challenge_digest,
                           std::vector<uint8_t> key_handle)
    : app_parameter_(std::move(app_parameter)),
      challenge_digest_(std::move(challenge_digest)),
      key_handle_(std::move(key_handle)) {}
U2fSignParam::U2fSignParam(U2fSignParam&& that) = default;
U2fSignParam& U2fSignParam::operator=(U2fSignParam&& that) = default;
U2fSignParam::~U2fSignParam() = default;

std::unique_ptr<apdu::ApduCommand> U2fSignParam::CreateU2fSignApduCommand()
    const {
  if (app_parameter_.size() != kAppIdDigestLen ||
      challenge_digest_.size() != kChallengeDigestLen ||
      key_handle_.size() > kMaxKeyHandleLength) {
    return nullptr;
  }

  auto command = std::make_unique<apdu::ApduCommand>();
  std::vector<uint8_t> data(challenge_digest_.begin(), challenge_digest_.end());
  data.insert(data.end(), app_parameter_.begin(), app_parameter_.end());
  data.push_back(static_cast<uint8_t>(key_handle_.size()));
  data.insert(data.end(), key_handle_.begin(), key_handle_.end());
  command->set_ins(kInsU2fSign);
  command->set_p1(check_only_ ? kP1CheckOnly : kP1TupRequiredConsumed);
  command->set_data(data);
  return command;
}

base::Optional<std::vector<uint8_t>> U2fSignParam::Encode() const {
  auto sign_cmd = CreateU2fSignApduCommand();
  if (!sign_cmd) {
    return base::nullopt;
  }
  return sign_cmd->GetEncodedCommand();
}

U2fSignParam& U2fSignParam::SetCheckOnly(bool check_only) {
  check_only_ = check_only;
  return *this;
}

}  // namespace device
