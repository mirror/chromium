// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_FIDO_U2F_VERSION_PARAM_H_
#define DEVICE_FIDO_U2F_VERSION_PARAM_H_

#include <stdint.h>

#include <memory>
#include <vector>

#include "base/optional.h"
#include "components/apdu/apdu_command.h"
#include "device/fido/fido_request_param.h"

namespace device {

class U2fVersionParam : public FidoRequestParam {
 public:
  U2fVersionParam();
  ~U2fVersionParam() override;

  // Creates Apdu command with format specified in the U2F spec. If request is
  // legacy version request, then two extra bytes are added as suffix.
  // https://fidoalliance.org/specs/fido-u2f-v1.0-nfc-bt-amendment-20150514/fido-u2f-raw-message-formats.html
  std::unique_ptr<apdu::ApduCommand> CreateU2fVersionApduCommand() const;

  // Returns APDU formatted serialized request to send to U2F authenticators.
  base::Optional<std::vector<uint8_t>> Encode() const override;

  U2fVersionParam& SetIsLegacyVersion(bool is_legacy_version);

 private:
  bool is_legacy_version_ = false;
};

}  // namespace device

#endif  // DEVICE_FIDO_U2F_VERSION_PARAM_H_
