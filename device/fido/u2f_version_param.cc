// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/u2f_version_param.h"

#include <utility>

#include "device/fido/ctap_constants.h"
#include "device/fido/u2f_apdu_command.h"

namespace device {

U2fVersionParam::U2fVersionParam() = default;
U2fVersionParam::U2fVersionParam(U2fVersionParam&& that) = default;
U2fVersionParam& U2fVersionParam::operator=(U2fVersionParam&& that) = default;
U2fVersionParam::~U2fVersionParam() = default;

base::Optional<std::vector<uint8_t>> U2fVersionParam::Encode() const {
  auto version_cmd = is_legacy_version_ ? U2fApduCommand::CreateVersion()
                                        : U2fApduCommand::CreateLegacyVersion();
  if (!version_cmd) {
    return base::nullopt;
  }
  return version_cmd->GetEncodedCommand();
}

U2fVersionParam& U2fVersionParam::SetIsLegacyVersion(bool is_legacy_version) {
  is_legacy_version_ = is_legacy_version;
  return *this;
}

}  // namespace device
