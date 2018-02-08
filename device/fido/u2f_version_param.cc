// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/u2f_version_param.h"

#include <utility>

#include "device/fido/ctap_constants.h"

namespace device {

U2fVersionParam::U2fVersionParam() = default;
U2fVersionParam::U2fVersionParam(U2fVersionParam&& that) = default;
U2fVersionParam& U2fVersionParam::operator=(U2fVersionParam&& that) = default;
U2fVersionParam::~U2fVersionParam() = default;

base::Optional<std::vector<uint8_t>> U2fVersionParam::Encode() const {
  auto version_cmd = CreateU2fVersionApduCommand();
  if (!version_cmd) {
    return base::nullopt;
  }
  return version_cmd->GetEncodedCommand();
}

std::unique_ptr<apdu::ApduCommand>
U2fVersionParam::CreateU2fVersionApduCommand() const {
  auto command = std::make_unique<apdu::ApduCommand>();
  command->set_ins(kInsU2fVersion);
  command->set_response_length(apdu::ApduCommand::kApduMaxResponseLength);

  // Early U2F drafts defined the U2F version command a format incompatible with
  // ISO 7816-4, so 2 additional 0x0 bytes are necessary.
  // https://fidoalliance.org/specs/fido-u2f-v1.1-id-20160915/fido-u2f-raw-message-formats-v1.1-id-20160915.html#implementation-considerations
  if (is_legacy_version_)
    command->set_suffix(std::vector<uint8_t>(2, 0));
  return command;
}

U2fVersionParam& U2fVersionParam::SetIsLegacyVersion(bool is_legacy_version) {
  is_legacy_version_ = is_legacy_version;
  return *this;
}

}  // namespace device
